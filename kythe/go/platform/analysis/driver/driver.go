/*
 * Copyright 2015 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Package driver contains a Driver implementation that sends analyses to a
// CompilationAnalyzer based on a Queue of compilations.
package driver

import (
	"context"
	"errors"
	"fmt"
	"io"
	"log"

	"kythe.io/kythe/go/platform/analysis"

	apb "kythe.io/kythe/proto/analysis_proto"
)

// A Compilation represents a compilation and other metadata needed to analyze it.
type Compilation struct {
	Unit     *apb.CompilationUnit // the compilation to analyze
	Revision string               // revision marker to attribute to the compilation
}

// CompilationFunc handles a single CompilationUnit.
type CompilationFunc func(context.Context, Compilation) error

// A Queue represents an ordered sequence of compilation units.
type Queue interface {
	// Next invokes f with the next available compilation in the queue.  If no
	// further values are available, Next must return io.EOF; otherwise, the
	// return value from f is propagated to the caller of Next.
	Next(_ context.Context, f CompilationFunc) error
}

// ErrRetry can be returned from a Driver's AnalysisError function to signal
// that the driver should retry the analysis immediately.
var ErrRetry = errors.New("retry analysis")

// Driver sends compilations sequentially from a queue to an analyzer.
type Driver struct {
	Analyzer        analysis.CompilationAnalyzer
	FileDataService string

	// Setup is called after a compilation has been pulled from the Queue and
	// before it is sent to the Analyzer (or Output is called).
	Setup CompilationFunc
	// Output is called for each analysis output returned from the Analyzer
	Output analysis.OutputFunc
	// Teardown is called after a compilation has been analyzed and there will be no further calls to Output.
	Teardown CompilationFunc

	// AnalysisError is called for each non-nil err returned from the Analyzer
	// (before Teardown is called).  The error returned from AnalysisError
	// replaces the analysis error that would normally be returned from Run.  If
	// ErrRetry is returned, the analysis is retried immediately.
	AnalysisError func(context.Context, Compilation, error) error
}

// IO is the IO subset of the analysis Driver struct.
type IO interface {
	// Setup is called after a compilation has been pulled from the Queue and
	// before it is sent to the Analyzer (or Output is called).
	Setup(context.Context, Compilation) error
	// Output is called for each analysis output returned from the Analyzer
	Output(context.Context, *apb.AnalysisOutput) error
	// Teardown is called after a compilation has been analyzed and there will be no further calls to Output.
	Teardown(context.Context, Compilation) error
	// AnalysisError is called for each non-nil err returned from the Analyzer
	// (before Teardown is called).  The error returned from AnalysisError
	// replaces the analysis error that would normally be returned from Run.  If
	// ErrRetry is returned, the analysis is retried immediately.
	AnalysisError(context.Context, Compilation, error) error
}

// Apply updates the Driver's IO functions to be that of the given interface.
func (d *Driver) Apply(io IO) {
	d.Setup = io.Setup
	d.Output = io.Output
	d.AnalysisError = io.AnalysisError
	d.Teardown = io.Teardown
}

func (d *Driver) validate() error {
	if d.Analyzer == nil {
		return errors.New("missing Analyzer")
	} else if d.Output == nil {
		return errors.New("missing Output function")
	}
	return nil
}

// Run sends each compilation received from the driver's Queue to the driver's
// Analyzer.  All outputs are passed to Output in turn.  An error is immediately
// returned if the Analyzer, Output, or Compilations fields are unset.
func (d *Driver) Run(ctx context.Context, queue Queue) error {
	if err := d.validate(); err != nil {
		return err
	}
	for {
		if err := queue.Next(ctx, func(ctx context.Context, cu Compilation) error {
			if d.Setup != nil {
				if err := d.Setup(ctx, cu); err != nil {
					return fmt.Errorf("analysis setup error: %v", err)
				}
			}
			err := ErrRetry
			for err == ErrRetry {
				err = d.Analyzer.Analyze(ctx, &apb.AnalysisRequest{
					Compilation:     cu.Unit,
					FileDataService: d.FileDataService,
					Revision:        cu.Revision,
				}, d.Output)
				if d.AnalysisError != nil && err != nil {
					err = d.AnalysisError(ctx, cu, err)
				}
			}
			if d.Teardown != nil {
				if tErr := d.Teardown(ctx, cu); tErr != nil {
					if err == nil {
						return fmt.Errorf("analysis teardown error: %v", tErr)
					}
					log.Printf("WARNING: analysis teardown error after analysis error: %v (analysis error: %v)", tErr, err)
				}
			}
			return err
		}); err == io.EOF {
			return nil
		} else if err != nil {
			return err
		}
	}
}
