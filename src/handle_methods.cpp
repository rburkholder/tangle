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
 * File:    handle_methods.cpp
 * Author:  raymond@burkholder.net
 * Project: boost.web
 * Created: July 22, 2026 18:36
 */

#include <string>

#include <boost/log/trivial.hpp>

#include "handle_methods.hpp"

namespace {
  static const std::string c_sVersion( "ounl web server with lua v1.0");
}

void response_bad_request( response_t& response, const boost::beast::string_view why ) {
  response.set( http::field::server, c_sVersion );
  response.set( http::field::content_type, "text/plain");
  response.body() = std::string( why );
}

void response_not_found( response_t& response, const boost::beast::string_view target ) {
  response.set( http::field::server, c_sVersion );
  response.set( http::field::content_type, "text/plain");
  response.body() = "The resource '" + std::string( target ) + "' was not found.\n";
}

void response_server_error( response_t& response, const boost::beast::string_view what ) {
  response.set( http::field::server, c_sVersion);
  response.set( http::field::content_type, "text/plain");
  response.body() = "An error occurred: '" + std::string( what ) + "'\n";
}

void resource_robots_txt( response_t& response ) {
  static const std::string content( "User-agent: *\nAllow: /\n" );
  response.set( http::field::server, c_sVersion );
  //response.set( http::field::content_type, "text/html");
  response.body() = content;
  response.content_length( content.size() );
}

void method_head( http::response<http::empty_body>& response ) {
  response.set( http::field::server, c_sVersion);
}

void method_get( http::response<http::file_body>& response ) {
  response.set( http::field::server, c_sVersion);
}

void method_post( response_t& response ) {
  response.set( http::field::server, c_sVersion);
  response.set( http::field::content_type, "text/plain");
  response.body() = "your post was accepted\n";
}

// TODO: convert read in to async operation, coroutine?, async load file(s), then pass as text based scripts
//   pre-compile to shared memory for re-use by luajit across sessions
//   use file change to match against cached contents, don't pre-cache though
void lua_method_get( std::string& path, sol_lua_t& sol_lua, http::response<http::string_body>& response ) {

  sol_lua().open_libraries( sol::lib::base, sol::lib::package, sol::lib::table, sol::lib::string );

  response.set( http::field::server, c_sVersion );

  sol_lua().set_function(
    "Render",
    [&response]( const std::string_view type, const std::string_view src ){
      response.set( http::field::content_type, type );
      response.body() = src;
      response.content_length( src.length() );
    } );

  try {
    auto result = sol_lua().safe_script_file(
      path,
      []( lua_State*, sol::protected_function_result pfr ){
        sol::error err = std::move( pfr );
        BOOST_LOG_TRIVIAL(error) << "An error (an expected one) occurred: " << err.what();
        return pfr;
      }
    );
    if ( result.valid() ) {
      return;
    }
    else {
      BOOST_LOG_TRIVIAL(error) << "lua script result: false";
    }
  }
  catch ( const sol::error& e ) {
    BOOST_LOG_TRIVIAL(error) << "lua script error: " << e.what();
  }

  // default error response
  response.result( http::status::not_found );
  response.set( http::field::content_type, "text/plain");
  response.body() = "The resource '" + path + "' has error\n";
  response.prepare_payload();

}

void lua_method_head( std::string& path, sol_lua_t& sol_lua, http::response<http::string_body>& response ) {

  //sol_lua.m_sol.open_libraries( sol::lib::base, sol::lib::package, sol::lib::table, sol::lib::string );

  response.set( http::field::server, c_sVersion );

  sol::load_result script;
  try {
    script = sol_lua().load_file( path, sol::load_mode::text );
    if ( script.valid() ) {
      response.set( http::field::content_type, "text/html" );
      response.content_length( 0 );
      return;
    }
  }
  catch ( const sol::error& e ) {
    BOOST_LOG_TRIVIAL(error) << "lua script error (1): " << e.what();
  }

  response.result( http::status::not_found );
  response.set( http::field::content_type, "text/plain");
  response.body() = "The resource '" + path + "' has error\n";
  response.prepare_payload();
}

void lua_method_post( std::string& path, sol_lua_t& sol_lua, http::response<http::string_body>& response ) {

  sol_lua().open_libraries( sol::lib::base, sol::lib::package, sol::lib::table, sol::lib::string );

  response.set( http::field::server, c_sVersion );

  sol_lua().set_function(
    "Render",
    [&response]( const std::string_view type, const std::string_view src ){
      response.set( http::field::content_type, type );
      response.body() = src;
      response.content_length( src.length() );
    } );

  // todo: pull out body variables and pass to script

  try {
    auto result = sol_lua().safe_script_file(
      path,
      []( lua_State*, sol::protected_function_result pfr ){
        sol::error err = std::move( pfr );
        BOOST_LOG_TRIVIAL(error) << "An error (an expected one) occurred: " << err.what();
        return pfr;
      }
    );
    if ( result.valid() ) {
      return;
    }
    else {
      BOOST_LOG_TRIVIAL(error) << "lua script result: false";
    }
  }
  catch ( const sol::error& e ) {
    BOOST_LOG_TRIVIAL(error) << "lua script error: " << e.what();
  }

  // default error response
  response.result( http::status::not_found );
  response.set( http::field::content_type, "text/plain");
  response.body() = "The resource '" + path + "' has error\n";
  response.prepare_payload();
}
