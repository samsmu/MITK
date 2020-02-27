#include <exception>
#include <iostream>

#include <AutoplanStreamSSL.h>

namespace Logger
{
  SinkSSL::SinkSSL(const std::string& certPath, const std::string& keyPath)
    : m_implContext(new ImplContext(boost::filesystem::exists(certPath) || boost::filesystem::exists(keyPath)))
    , m_implStream(new ImplStream(m_implContext))
  {
    if (boost::filesystem::exists(keyPath)) {
      m_implContext->ssl_context.load_verify_file(keyPath);
      //ssl_context.use_certificate_chain_file(certPath);
      //ssl_context.use_private_key_file(keyPath, boost::asio::ssl::context::pem);
      //m_implStream->ssl_context.set_default_verify_paths();
      //m_implStream->ssl_context.set_verify_mode(boost::asio::ssl::verify_none);
    }
  }

  SinkSSL::~SinkSSL()
  {
  }

  void SinkSSL::connect(const std::string& iphost, const std::string& ipport) const
  try {
    const boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(iphost), boost::lexical_cast<int>(ipport));
    m_implContext->ssl_stream.next_layer().connect(ep);
  } catch (const std::exception& e) {
    std::cout << "SinkSSL::connect fail: " << e.what() << std::endl;
  }
}
