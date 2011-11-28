

#pragma once

/**
 * Abstract class representing an SEI message with lightweight RTTI.
 */
class SEI
{
public:
  enum PayloadType {
    USER_DATA_UNREGISTERED = 5,
    PICTURE_DIGEST = 256,
  };
  
  SEI() {}
  virtual ~SEI() {}
  
  virtual PayloadType payloadType() const = 0;
};

class SEIuserDataUnregistered : public SEI
{
public:
  PayloadType payloadType() const { return USER_DATA_UNREGISTERED; }

  SEIuserDataUnregistered()
    : userData(0)
    {}

  virtual ~SEIuserDataUnregistered()
  {
    delete userData;
  }

  unsigned char uuid_iso_iec_11578[16];
  unsigned userDataLength;
  unsigned char *userData;
};

class SEIpictureDigest : public SEI
{
public:
  PayloadType payloadType() const { return PICTURE_DIGEST; }

  SEIpictureDigest() {}
  virtual ~SEIpictureDigest() {}
  
  enum Method {
    MD5,
    RESERVED,
  } method;

  unsigned char digest[16];
};

/**
 * A structure to collate all SEI messages.  This ought to be replaced
 * with a list of std::list<SEI*>.  However, since there is only one
 * user of the SEI framework, this will do initially */
class SEImessages
{
public:
  SEImessages()
    : user_data_unregistered(0)
    , picture_digest(0)
    {}

  ~SEImessages()
  {
    delete user_data_unregistered;
    delete picture_digest;
  }

  SEIuserDataUnregistered* user_data_unregistered;
  SEIpictureDigest* picture_digest;
};
