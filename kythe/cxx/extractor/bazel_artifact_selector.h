/*
 * Copyright 2020 The Kythe Authors. All rights reserved.
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
#ifndef KYTHE_CXX_EXTRACTOR_BAZEL_ARTIFACT_SELECTOR_H_
#define KYTHE_CXX_EXTRACTOR_BAZEL_ARTIFACT_SELECTOR_H_

#include <functional>
#include <memory>
#include <type_traits>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/meta/type_traits.h"
#include "absl/status/status.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "google/protobuf/any.pb.h"
#include "kythe/cxx/common/regex.h"
#include "kythe/cxx/extractor/bazel_artifact.h"
#include "re2/re2.h"
#include "src/main/java/com/google/devtools/build/lib/buildeventstream/proto/build_event_stream.pb.h"

namespace kythe {

/// \brief BazelArtifactSelector is an interface which can be used for finding
/// extractor artifacts in a Bazel sequence of build_event_stream.BuildEvent
/// messages.
class BazelArtifactSelector {
 public:
  virtual ~BazelArtifactSelector() = default;

  /// \brief Selects matching BazelArtifacts from the provided event.
  /// Select() will be called for each message in the stream to allow
  /// implementations to update internal state.
  virtual absl::optional<BazelArtifact> Select(
      const build_event_stream::BuildEvent& event) = 0;

  /// \brief Encodes per-stream selector state into the Any protobuf.
  /// Stateful selectors should serialize any per-stream state into a
  /// suitable protocol buffer, encoded as an Any. If no state has been
  /// accumulated, they should return an empty protocol buffer of the
  /// appropriate type and return true.
  /// Stateless selectors should return false.
  virtual bool SerializeInto(google::protobuf::Any& state) const {
    return false;
  }

  /// \brief Updates any per-stream state from the provided proto.
  /// Stateless selectors should unconditionally return a kUnimplemented status.
  /// Stateful selectors should return OK if the provided state contains a
  /// suitable proto, InvalidArgument if the proto is of the right type but
  /// cannot be decoded or FailedPrecondition if the proto is of the wrong type.
  virtual absl::Status DeserializeFrom(const google::protobuf::Any& state) {
    return absl::UnimplementedError("stateless selector");
  }

  /// \brief Finds and updates any per-stream state from the provided list.
  /// Returns OK if the selector is stateless or if the requisite state was
  /// found in the list.
  /// Returns NotFound for a stateful selector whose state was not present
  /// or InvalidArgument if the state was present but couldn't be decoded.
  absl::Status Deserialize(absl::Span<const google::protobuf::Any> state);
  absl::Status Deserialize(
      absl::Span<const google::protobuf::Any* const> state);

 protected:
  // Not publicly copyable or movable to avoid slicing, but subclasses may be.
  BazelArtifactSelector() = default;
  BazelArtifactSelector(const BazelArtifactSelector&) = default;
  BazelArtifactSelector& operator=(const BazelArtifactSelector&) = default;
};

/// \brief A type-erased value-type implementation of the BazelArtifactSelector
/// interface.
class AnyArtifactSelector final : public BazelArtifactSelector {
 public:
  /// \brief Constructs an AnyArtifactSelector which delegates to the provided
  /// argument, which must derive from BazelArtifactSelector.
  template <
      typename S,
      typename = absl::enable_if_t<!std::is_same_v<S, AnyArtifactSelector>>,
      typename =
          absl::enable_if_t<std::is_convertible_v<S&, BazelArtifactSelector&>>>
  AnyArtifactSelector(S s)
      : AnyArtifactSelector([s = std::move(s)]() mutable -> S& { return s; }) {}

  // Copyable.
  AnyArtifactSelector(const AnyArtifactSelector&) = default;
  AnyArtifactSelector& operator=(const AnyArtifactSelector&) = default;

  /// \brief AnyArtifactSelector is movable, but will be empty after a move.
  /// The only valid operations on an empty AnyArtifactSelector is assigning a
  /// new value or destruction.
  AnyArtifactSelector(AnyArtifactSelector&&) = default;
  AnyArtifactSelector& operator=(AnyArtifactSelector&&) = default;

  /// \brief Forwards selection to the contained BazelArtifactSelector.
  absl::optional<BazelArtifact> Select(
      const build_event_stream::BuildEvent& event) {
    return get_().Select(event);
  }

  /// \brief Forwards serialization to the contained BazelArtifactSelector.
  bool SerializeInto(google::protobuf::Any& state) const final {
    return get_().SerializeInto(state);
  }

  /// \brief Forwards deserialization to the contained BazelArtifactSelector.
  absl::Status DeserializeFrom(const google::protobuf::Any& state) final {
    return get_().DeserializeFrom(state);
  }

 private:
  explicit AnyArtifactSelector(std::function<BazelArtifactSelector&()> get)
      : get_(std::move(get)) {}

  std::function<BazelArtifactSelector&()> get_;
};

/// \brief Options class used for constructing an AspectArtifactSelector.
struct AspectArtifactSelectorOptions {
  // A set of patterns used to filter file names from NamedSetOfFiles events.
  // Matches nothing by default.
  RegexSet file_name_allowlist;
  // A set of patterns used to filter output_group names from TargetComplete
  // events. Matches nothing by default.
  RegexSet output_group_allowlist;
  // A set of patterns used to filter aspect names from TargetComplete events.
  RegexSet target_aspect_allowlist = RegexSet::Build({".*"}).value();
};

/// \brief A BazelArtifactSelector implementation which tracks state from
/// NamedSetOfFiles and TargetComplete events to select artifacts produced by
/// extractor aspects.
class AspectArtifactSelector final : public BazelArtifactSelector {
 public:
  using Options = AspectArtifactSelectorOptions;

  /// \brief Constructs an instance of AspectArtifactSelector from the provided
  /// options.
  explicit AspectArtifactSelector(Options options)
      : options_(std::move(options)) {}

  /// \brief Selects an artifact if the event matches an expected
  /// aspect-produced compilation unit.
  absl::optional<BazelArtifact> Select(
      const build_event_stream::BuildEvent& event) final;

  /// \brief Serializes the accumulated state into the return value, which will
  /// always be non-empty and of type
  /// `kythe.proto.BazelAspectArtifactSelectorState`.
  bool SerializeInto(google::protobuf::Any& state) const final;

  /// \brief Deserializes accumulated stream state from an Any of type
  /// `kythe.proto.BazelAspectArtifactSelectorState`.
  absl::Status DeserializeFrom(const google::protobuf::Any& state) final;

 private:
  struct State {
    // A record of all of the NamedSetOfFiles events which have been processed.
    absl::flat_hash_set<std::string> disposed;
    // Mapping from fileset id to NamedSetOfFiles whose file names matched the
    // allowlist, but have not yet been consumed by an event.
    absl::flat_hash_map<std::string, build_event_stream::NamedSetOfFiles>
        filesets;
    // Mapping from fileset id to target name which required that
    // file set when it had not yet been seen.
    absl::flat_hash_map<std::string, std::string> pending;
  };
  absl::optional<BazelArtifact> SelectFileSet(
      absl::string_view id, const build_event_stream::NamedSetOfFiles& fileset);

  absl::optional<BazelArtifact> SelectTargetCompleted(
      const build_event_stream::BuildEventId::TargetCompletedId& id,
      const build_event_stream::TargetComplete& payload);

  void ReadFilesInto(absl::string_view id, absl::string_view target,
                     std::vector<BazelArtifactFile>& files);

  Options options_;
  State state_;
};

/// \brief An ArtifactSelector which selects artifacts emitted by extra
/// actions.
///
/// This will select any successful ActionCompleted build event, but the
/// selection can be restricted to an allowlist of action_types.
class ExtraActionSelector final : public BazelArtifactSelector {
 public:
  /// \brief Constructs an ExtraActionSelector from an allowlist against which
  /// to match ActionCompleted events. An empty set will select any successful
  /// action.
  explicit ExtraActionSelector(
      absl::flat_hash_set<std::string> action_types = {});

  /// \brief Constructs an ExtraActionSelector from an allowlist pattern.
  /// Both a null and an empty pattern will match nothing.
  explicit ExtraActionSelector(const RE2* action_pattern);

  /// \brief Selects artifacts from ExtraAction-based extractors.
  absl::optional<BazelArtifact> Select(
      const build_event_stream::BuildEvent& event) final;

 private:
  std::function<bool(absl::string_view)> action_matches_;
};

}  // namespace kythe

#endif  // KYTHE_CXX_EXTRACTOR_BAZEL_ARTIFACT_SELECTOR_H_
