#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>

namespace Logger
{
  class SinkSSL : private boost::noncopyable
  {
  public:
    SinkSSL(const std::string& certPath, const std::string& keyPath);
    ~SinkSSL();

    void connect(const std::string& iphost, const std::string& ipport) const;
    void add_stream(boost::log::sinks::text_ostream_backend& backend) const;

  private:
    struct ImplContext;
    struct ImplStream;

    boost::shared_ptr<ImplContext> m_implContext;
    boost::shared_ptr<ImplStream> m_implStream;
  };
}