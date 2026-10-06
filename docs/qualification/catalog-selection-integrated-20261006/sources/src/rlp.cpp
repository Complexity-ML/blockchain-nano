#include "nano/rlp.hpp"
namespace nano {
Int Rlp::number() const { const auto& b=blob();if(b.size()>32 || (!b.empty()&&b[0]==0))throw std::invalid_argument("noncanonical RLP integer");return integer(b); }
const Bytes& Rlp::blob() const {if(list)throw std::invalid_argument("RLP list where bytes expected");return data;}
const Rlp& Rlp::operator[](size_t i) const { if(!list)throw std::invalid_argument("RLP bytes where list expected");return items.at(i); }
static Bytes prefix(Bytes data,uint8_t small,uint8_t large) { if(data.size()<56) {data.insert(data.begin(),uint8_t(small+data.size()));return data;} auto len=bytes(Int(data.size()));return concat(concat(Bytes{uint8_t(large+len.size())},len),data); }
Bytes Rlp::encode() const { if(!list){if(data.size()==1&&data[0]<128)return data;return prefix(data,128,183);}Bytes all;for(const auto& i:items)all=concat(std::move(all),i.encode());return prefix(std::move(all),192,247); }
static Rlp parse(const Bytes& in,size_t& pos,size_t limit,size_t depth) {
 if(depth>64||pos>=limit)throw std::invalid_argument("invalid RLP nesting/length");uint8_t tag=in[pos++];if(tag<128)return Rlp(Bytes{tag});bool list=tag>=192;size_t len=0;
 if(tag<184)len=tag-128;else if(tag<192){size_t n=tag-183;if(n>sizeof(size_t)||n>limit-pos||in[pos]==0)throw std::invalid_argument("bad RLP length");for(size_t i=0;i<n;++i)len=(len<<8)|in[pos++];if(len<56)throw std::invalid_argument("noncanonical RLP");}
 else if(tag<248)len=tag-192;else{size_t n=tag-247;if(n>sizeof(size_t)||n>limit-pos||in[pos]==0)throw std::invalid_argument("bad RLP length");for(size_t i=0;i<n;++i)len=(len<<8)|in[pos++];if(len<56)throw std::invalid_argument("noncanonical RLP");}
 if(len>limit-pos)throw std::invalid_argument("truncated RLP");size_t end=pos+len;
 if(!list){if(len==1&&in[pos]<128)throw std::invalid_argument("noncanonical RLP byte");Bytes b(in.begin()+pos,in.begin()+end);pos=end;return Rlp(b);}
 std::vector<Rlp> out;while(pos<end)out.push_back(parse(in,pos,end,depth+1));return Rlp(out);
}
Rlp Rlp::decode(const Bytes& input){size_t pos=0;auto r=parse(input,pos,input.size(),0);if(pos!=input.size())throw std::invalid_argument("trailing RLP");return r;}
using Entries=std::vector<std::pair<Bytes,Bytes>>;
static Bytes compact(const Bytes& key,size_t off,size_t count,bool leaf){Bytes out;bool odd=count%2;out.push_back(uint8_t((leaf?32:0)+(odd?16+key[off++]:0)));for(size_t i=odd?1:0;i<count;i+=2){out.push_back(uint8_t(key[off]*16+key[off+1]));off+=2;}return out;}
static Rlp trie_node(const Entries& e,size_t off);
static Rlp reference(const Rlp& node){auto raw=node.encode();return raw.size()<32?node:Rlp(keccak(raw));}
static Rlp trie_node(const Entries& e,size_t off){
 if(e.size()==1)return Rlp(std::vector<Rlp>{Rlp(compact(e[0].first,off,e[0].first.size()-off,true)),Rlp(e[0].second)});
 size_t common=off;while(common<e.front().first.size()&&common<e.back().first.size()&&e.front().first[common]==e.back().first[common])++common;
 if(common>off)return Rlp(std::vector<Rlp>{Rlp(compact(e[0].first,off,common-off,false)),reference(trie_node(e,common))});
 std::vector<Rlp> children(17);for(const auto& [k,v]:e)if(k.size()==off)children[16]=Rlp(v);
 for(unsigned nib=0;nib<16;++nib){Entries group;for(const auto& p:e)if(p.first.size()>off&&p.first[off]==nib)group.push_back(p);if(!group.empty())children[nib]=reference(trie_node(group,off+1));}return Rlp(children);
}
Bytes trie_root(const std::map<Bytes,Bytes>& entries){Entries e;for(const auto& [key,val]:entries){if(val.empty())continue;Bytes n;for(auto b:key){n.push_back(b>>4);n.push_back(b&15);}e.emplace_back(n,val);}return e.empty()?keccak(Rlp(Bytes{}).encode()):keccak(trie_node(e,0).encode());}
}
