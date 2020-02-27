#pragma once

#include <MitkLoggingExports.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>

#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

namespace Logger
{
  template <typename TStraem>
  class MITKLOGGING_EXPORT ssl_iostream_device : public boost::iostreams::device<boost::iostreams::bidirectional> {
  public:
    ssl_iostream_device(TStraem &stream, bool use_ssl = true)
      : m_stream(stream)
      , m_need_handshake(use_ssl)
      , m_use_ssl(use_ssl)
    {
    }

    std::streamsize read(char* s, std::streamsize n)
    {
      handshake(boost::asio::ssl::stream_base::server); // HTTPS servers read first
      return m_use_ssl
        ? m_stream.read_some(boost::asio::buffer(s, n))
        : m_stream.next_layer().read_some(boost::asio::buffer(s, n));
    }

    std::streamsize write(const char* s, std::streamsize n)
    {
      handshake(boost::asio::ssl::stream_base::client); // HTTPS clients write first
      return m_use_ssl
        ? boost::asio::write(m_stream, boost::asio::buffer(s, n))
        : boost::asio::write(m_stream.next_layer(), boost::asio::buffer(s, n));
    }

    bool use_ssl() const
    {
      return m_use_ssl;
    }

  private:
    void handshake(boost::asio::ssl::stream_base::handshake_type role)
    {
      if (!m_need_handshake) {
        return;
      }
      m_need_handshake = false;
      m_stream.handshake(role);
    }

    const bool m_use_ssl;
    bool m_need_handshake;
    TStraem& m_stream;
  };

  struct MITKLOGGING_EXPORT ImplContext : private boost::noncopyable
  {
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_stream_t;
    typedef ssl_iostream_device<ssl_stream_t> ssl_iostream_device_t;

    boost::asio::ssl::context ssl_context;
    boost::asio::io_service io_context;
    ssl_stream_t ssl_stream;
    ssl_iostream_device_t ssl_device;

    explicit ImplContext(bool use_ssl = true)
      : ssl_context(boost::asio::ssl::context::sslv23)
      , ssl_stream(io_context, ssl_context)
      , ssl_device(ssl_stream, use_ssl)
    {
    }
  };

  struct MITKLOGGING_EXPORT ImplStream : private boost::noncopyable
  {
    typedef boost::iostreams::stream<ImplContext::ssl_iostream_device_t> ssl_iostreams_t;

    boost::shared_ptr<ImplContext> ssl_context;
    boost::shared_ptr<ssl_iostreams_t> ssl_iostream;

    explicit ImplStream(const boost::shared_ptr<ImplContext>& context)
      : ssl_context(context)
      , ssl_iostream(ssl_context, new ssl_iostreams_t(context->ssl_device))
    {
    }
  };

  class MITKLOGGING_EXPORT SinkSSL : private boost::noncopyable
  {
  public:
    SinkSSL(const std::string& certPath, const std::string& keyPath);
    ~SinkSSL();

    void connect(const std::string& iphost, const std::string& ipport) const;
    void add_stream(boost::log::sinks::text_ostream_backend& backend) const {
      backend.add_stream(m_implStream->ssl_iostream);
    }

  private:
    boost::shared_ptr<ImplContext> m_implContext;
    boost::shared_ptr<ImplStream> m_implStream;
  };
}
