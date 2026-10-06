#pragma once
#include "nano/storage.hpp"
#include <functional>
namespace nano::capella_checkpoint_detail {
struct FragmentCheckpoint {std::string manifest;std::vector<CheckpointBlob> blobs;};
// LOCAL persistence transform only. Its input must be the canonical output of
// the fully validated Capella durable serializer; this is not peer admission.
// Restoration of expanded bytes must still run restore_durable afterwards.
inline FragmentCheckpoint fragment_checkpoint(std::string_view canonical,size_t maximum_bytes,
 size_t maximum_blocks,const std::function<void()>& check={}){
 auto require=[](bool ok){if(!ok)throw std::invalid_argument("Invalid Capella fragment checkpoint");};
 require(!canonical.empty()&&canonical.size()<=maximum_bytes);if(check)check();
 auto document=Json::parse(canonical);require(document.is_object()&&document.dump()==canonical);
 auto& store=document.contains("store")?document.at("store"):document;
 require(store.is_object()&&store.contains("blocks")&&store.at("blocks").is_array());
 const auto& rows=store.at("blocks");require(!rows.empty()&&rows.size()<=maximum_blocks);
 FragmentCheckpoint output;Json references=Json::array();std::string previous;size_t total=0;
 for(const auto& row:rows){
  if(check)check();require(row.is_array()&&row.size()==6&&row[0].is_string());
  const auto root=row[0].get<std::string>();require(root.size()==66&&hex(unhex(root))==root&&root>previous);previous=root;
  auto payload=row.dump();require(payload.size()<=maximum_bytes-total);total+=payload.size();
  auto digest=sha256(Bytes(payload.begin(),payload.end()));
  references.push_back(Json::array({root,hex(digest),quantity(Int(std::to_string(payload.size())))}));
  output.blobs.push_back({std::move(digest),std::move(payload)});
 }
 store["blocks"]=std::move(references);
 output.manifest=Json{{"format","nano-capella-fragments-v1"},{"expandedBytes",quantity(Int(std::to_string(canonical.size())))},{"expandedDigest",hex(sha256(Bytes(canonical.begin(),canonical.end())))},{"snapshot",std::move(document)}}.dump();
 require(output.manifest.size()<=maximum_bytes);if(check)check();return output;
}
// Authenticate the manifest BEFORE parsing any references. Fetch receives a
// committed hash and an exact length bound; absent/changed data is never guessed
// from another branch or primary store. No write or signing occurs here.
inline std::string expand_fragment_checkpoint(std::string_view manifest,const Bytes& expected_digest,
 size_t maximum_bytes,size_t maximum_blocks,
 const std::function<std::optional<std::string>(const Bytes&,size_t)>& fetch,
 const std::function<void()>& check={}){
 auto require=[](bool ok){if(!ok)throw std::invalid_argument("Invalid Capella fragment checkpoint");};
 require(!manifest.empty()&&manifest.size()<=maximum_bytes&&expected_digest.size()==32);
 require(sha256(Bytes(manifest.begin(),manifest.end()))==expected_digest);if(check)check();
 auto envelope=Json::parse(manifest);require(envelope.is_object()&&envelope.size()==4&&envelope.at("format")=="nano-capella-fragments-v1"&&envelope.dump()==manifest);
 const auto& encoded_size=envelope.at("expandedBytes");require(encoded_size.is_string());const auto length=u64(quantity(encoded_size));require(length&&length<=maximum_bytes&&quantity(Int(std::to_string(length)))==encoded_size.get<std::string>());
 const auto& encoded_digest=envelope.at("expandedDigest");require(encoded_digest.is_string());const auto whole_digest=unhex(encoded_digest.get<std::string>());require(whole_digest.size()==32&&hex(whole_digest)==encoded_digest.get<std::string>());
 auto document=std::move(envelope.at("snapshot"));require(document.is_object());auto& store=document.contains("store")?document.at("store"):document;
 require(store.is_object()&&store.contains("blocks")&&store.at("blocks").is_array());auto references=std::move(store.at("blocks"));require(!references.empty()&&references.size()<=maximum_blocks);
 Json rows=Json::array();std::string previous;size_t total=0;
 for(const auto& reference:references){
  if(check)check();require(reference.is_array()&&reference.size()==3&&reference[0].is_string()&&reference[1].is_string()&&reference[2].is_string());
  const auto root=reference[0].get<std::string>(),encoded=reference[1].get<std::string>();require(root.size()==66&&hex(unhex(root))==root&&root>previous);previous=root;
  const auto digest=unhex(encoded);require(digest.size()==32&&hex(digest)==encoded);
  const auto bytes=u64(quantity(reference[2]));require(bytes&&bytes<=length-total&&quantity(Int(std::to_string(bytes)))==reference[2].get<std::string>());total+=bytes;
  const auto payload=fetch(digest,size_t(bytes));require(payload&&payload->size()==bytes&&sha256(Bytes(payload->begin(),payload->end()))==digest);
  if(check)check();auto row=Json::parse(*payload);require(row.is_array()&&row.size()==6&&row[0]==root&&row.dump()==*payload);rows.push_back(std::move(row));
 }
 store["blocks"]=std::move(rows);auto expanded=document.dump();require(expanded.size()==length&&sha256(Bytes(expanded.begin(),expanded.end()))==whole_digest);if(check)check();return expanded;
}
}
