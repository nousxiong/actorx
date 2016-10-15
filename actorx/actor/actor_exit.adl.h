#ifndef actorx_adl_actor_exit_adl_h_adata_header_define
#define actorx_adl_actor_exit_adl_h_adata_header_define

#include <adata/adata.hpp>

namespace actorx {namespace adl {
  struct actor_exit
  {
    int8_t type;
    ::std::string errmsg;
    actor_exit()
    :    type(0)
    {}
  };

}}

namespace adata
{
template<>
struct is_adata<actorx::adl::actor_exit>
{
  static const bool value = true;
};

}
namespace adata
{
  template<typename stream_ty>
  ADATA_INLINE void read( stream_ty& stream, ::actorx::adl::actor_exit& value)
  {
    ::std::size_t offset = stream.read_length();
    int64_t tag = 0;
    read(stream,tag);
    int32_t len_tag = 0;
    read(stream,len_tag);

    if(tag&1LL)    {read(stream,value.type);}
    if(tag&2LL)    {
      int32_t len = check_read_size(stream);
      value.errmsg.resize(len);
      stream.read((char *)value.errmsg.data(),len);
    }
    if(len_tag >= 0)
    {
      ::std::size_t read_len = stream.read_length() - offset;
      ::std::size_t len = (::std::size_t)len_tag;
      if(len > read_len) stream.skip_read(len - read_len);
    }
  }

  template <typename stream_ty>
  ADATA_INLINE void skip_read(stream_ty& stream, ::actorx::adl::actor_exit* )
  {
    skip_read_compatible(stream);
  }

  ADATA_INLINE int32_t size_of(const ::actorx::adl::actor_exit& value)
  {
    int32_t size = 0;
    int64_t tag = 1LL;
    if(!value.errmsg.empty()){tag|=2LL;}
    {
      size += size_of(value.type);
    }
    if(tag&2LL)
    {
      {
        int32_t len = (int32_t)value.errmsg.size();
        size += size_of(len);
        size += len;
      }
    }
    size += size_of(tag);
    size += size_of(size + size_of(size));
    return size;
  }

  template<typename stream_ty>
  ADATA_INLINE void write(stream_ty& stream , const ::actorx::adl::actor_exit& value)
  {
    int64_t tag = 1LL;
    if(!value.errmsg.empty()){tag|=2LL;}
    write(stream,tag);
    write(stream,size_of(value));
    write(stream,value.type);
    if(tag&2LL)    {
      int32_t len = (int32_t)(value.errmsg).size();
      write(stream,len);
      stream.write((value.errmsg).data(),len);
    }
  }

  template<typename stream_ty>
  ADATA_INLINE void raw_read( stream_ty& stream, ::actorx::adl::actor_exit& value)
  {
    read(stream,value.type);
    {
      int32_t len = check_read_size(stream);
      value.errmsg.resize(len);
      stream.read((char *)value.errmsg.data(),len);
    }
  }

  ADATA_INLINE int32_t raw_size_of(const ::actorx::adl::actor_exit& value)
  {
    int32_t size = 0;
    size += size_of(value.type);
    {
      int32_t len = (int32_t)value.errmsg.size();
      size += size_of(len);
      size += len;
    }
    return size;
  }

  template<typename stream_ty>
  ADATA_INLINE void raw_write(stream_ty& stream , const ::actorx::adl::actor_exit& value)
  {
    write(stream,value.type);
    {
      int32_t len = (int32_t)(value.errmsg).size();
      write(stream,len);
      stream.write((value.errmsg).data(),len);
    }
  }

}

#endif
