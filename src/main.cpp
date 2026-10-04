/************************************************************************
 * Copyright(c) 2026, One Unified. All rights reserved.                 *
 * email: info@oneunified.net                                           *
 *                                                                      *
 * This file is provided as is WITHOUT ANY WARRANTY                     *
 *  without even the implied warranty of                                *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                *
 *                                                                      *
 * This software may not be used nor distributed without proper license *
 * agreement.                                                           *
 *                                                                      *
 * See the file LICENSE.txt for redistribution information.             *
 ************************************************************************/

/*
 * File:    main.cpp
 * Author:  raymond@burkholder.net
 * Project: boost.web
 * Created: July 20, 2026 18:36
 */

//------------------------------------------------------------------------------
//
// Example: Advanced server, flex (plain + SSL)
//
//------------------------------------------------------------------------------

#include <thread>
#include <vector>

#include <boost/log/trivial.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/beast.hpp>

#include "config.hpp"
#include "listen.hpp"
#include "task_group.hpp"
#include "handle_signals.hpp"
#include "server_certificate.hpp"

namespace net       = boost::asio;
namespace ssl       = boost::asio::ssl;

using executor_type = net::strand<net::io_context::executor_type>;

int main( int argc, char* argv[] ) {

  std::string sConfigFilename( "tangle.cfg" );

  BOOST_LOG_TRIVIAL(info) << "(c)2026 One Unified Net Limited";

  // Check command line arguments.
  if( 2 == argc ) {
    sConfigFilename = argv[ 1 ];
  }

  config::Values choices;

  if ( Load( sConfigFilename, choices ) ) {}
  else {
    return EXIT_FAILURE;
  }

  auto const listen_address  = net::ip::make_address( choices.sListenAddress );
  auto const listen_endpoint = net::ip::tcp::endpoint{ listen_address, choices.nPortHttps };

  const uint16_t nThreads = std::thread::hardware_concurrency();
  if ( choices.nThreads == nThreads ) {}
  else {
    BOOST_LOG_TRIVIAL(warning)
      << "suggested hardware maximum threads (" << nThreads << "), "
      << "configuration specified threads (" << choices.nThreads << ")";
  }

  // The io_context is required for all I/O
  net::io_context ioc{ choices.nThreads };

  // The SSL context is required, and holds certificates
  //ssl::context ssl_ctx{ ssl::context::tlsv12 };
  ssl::context ssl_ctx{ ssl::context::tlsv13 };

  // This holds the certificate used by the server
  if ( ( 0 < choices.sCertificatePathFullChain.size() ) && ( 0 < choices.sCertfificatePathPrivKey.size() ) ) {
    load_server_certificate( ssl_ctx, choices.sCertificatePathFullChain, choices.sCertfificatePathPrivKey );
  }

  // Track coroutines
  task_group task_group{ ioc.get_executor() };

  // Create and launch a listening coroutine
  net::co_spawn(
    net::make_strand( ioc ),  // the strand can be removed to allow access to all threads
    listen( task_group, ssl_ctx, listen_endpoint, choices ),
    task_group.adapt(
      []( std::exception_ptr e ) {
        if( e ) {
          try {
              std::rethrow_exception(e);
          }
          catch ( std::exception& e ) {
            BOOST_LOG_TRIVIAL(error) << "Error in listener: " << e.what();
          }
        }
      })
    );

  // Create and launch a signal handler coroutine
  net::co_spawn(
    net::make_strand( ioc ),
    handle_signals( task_group ), net::detached
  );

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> vThread;
  vThread.reserve( choices.nThreads - 1 );
  for( auto i = choices.nThreads - 1; i > 0; --i ) {
    vThread.emplace_back( [&ioc] { ioc.run(); } );
  }

  ioc.run();

  // Block until all the threads exit
  for ( auto& thread : vThread )
    if( thread.joinable() ) thread.join();

  return EXIT_SUCCESS;
}

