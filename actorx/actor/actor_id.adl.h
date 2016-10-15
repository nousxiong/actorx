#ifndef actorx_adl_actor_id_adl_h_adata_header_define
#define actorx_adl_actor_id_adl_h_adata_header_define

#include <adata/adata.hpp>

namespace actorx {namespace adl {
  struct actor_id
  {
    int64_t axid;
    int64_t timestamp;
    int64_t id;
    int64_t sid;
    actor_id()
    :    axid(0LL),
    timestamp(0LL),
    id(0LL),
    sid(0LL)
    {}
  };

}}

namespace adata
{
template<>
struct is_adata<actorx::adl::actor_id>
{
  static const bool value = true;
};

}
namespace adata
{
  template<typename stream_ty>
  ADATA_INLINE void read( stream_ty& stream, ::actorx::adl::actor_id& value)
  {
    ::std::size_t offset = stream.read_length();
    int64_t tag = 0;
    read(stream,tag);
    int32_t len_tag = 0;
    read(stream,len_tag);

    if(tag&1LL)    {read(stream,value.axid);}
    if(tag&2LL)    {read(stream,value.timestamp);}
    if(tag&4LL)    {read(stream,value.id);}
    if(tag&8LL)    {read(stream,value.sid);}
    if(len_tag >= 0)
    {
      ::std::size_t read_len = stream.read_length() - offset;
      ::std::size_t len = (::std::size_t)len_tag;
      if(len > read_len) stream.skip_read(len - read_len);
    }
  }

  template <typename stream_ty>
  ADATA_INLINE void skip_read(stream_ty& stream, ::actorx::adl::actor_id* )
  {
    skip_read_compatible(stream);
  }

  ADATA_INLINE int32_t size_of(const ::actorx::adl::actor_id& value)
  {
    int32_t size = 0;
    int64_t tag = 15LL;
    {
      size += size_of(value.axid);
    }
    {
      size += size_of(value.timestamp);
    }
    {
      size += size_of(value.id);
    }
    {
      size += size_of(value.sid);
    }
    size += size_of(tag);
    size += size_of(size + size_of(size));
    return size;
  }

  template<typename stream_ty>
  ADATA_INLINE void write(stream_ty& stream , const ::actorx::adl::actor_id& value)
  {
    int64_t tag = 15LL;
    write(stream,tag);
    write(stream,size_of(value));
    write(stream,value.axid);
    write(stream,value.timestamp);
    write(stream,value.id);
    write(stream,value.sid);
  }

  template<typename stream_ty>
  ADATA_INLINE void raw_read( stream_ty& stream, ::actorx::adl::actor_id& value)
  {
    read(stream,value.axid);
    read(stream,value.timestamp);
    read(stream,value.id);
    read(stream,value.sid);
  }

  ADATA_INLINE int32_t raw_size_of(const ::actorx::adl::actor_id& value)
  {
    int32_t size = 0;
    size += size_of(value.axid);
    size += size_of(value.timestamp);
    size += size_of(value.id);
    size += size_of(value.sid);
    return size;
  }

  template<typename stream_ty>
  ADATA_INLINE void raw_write(stream_ty& stream , const ::actorx::adl::actor_id& value)
  {
    write(stream,value.axid);
    write(stream,value.timestamp);
    write(stream,value.id);
    write(stream,value.sid);
  }

}

#endif
