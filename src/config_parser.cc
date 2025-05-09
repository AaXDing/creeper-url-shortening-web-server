// An nginx config file parser.
//
// See:
//   http://wiki.nginx.org/Configuration
//   http://blog.martinfjordvald.com/2010/07/nginx-primer/
//
// How Nginx does it:
//   http://lxr.nginx.org/source/src/core/ngx_conf_file.c

#include "config_parser.h"

#include <boost/filesystem.hpp>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "logging.h"

std::string NginxConfig::to_string(int depth) {
  std::string serialized_config;
  for (const auto& statement : statements_) {
    serialized_config.append(statement->to_string(depth));
  }
  return serialized_config;
}

int NginxConfig::get_port() const {
  for (const auto& statement : statements_) {
    // Check if the statement is a "port" directive
    if (statement->tokens_.size() == 2 && statement->tokens_[0] == "port") {
      try {
        int port = std::stoi(statement->tokens_[1]);
        LOG(info) << "Found port directive, port=" << port;
        return port;
      } catch (std::exception& e) {
        // If the conversion fails, return -1
        LOG(error) << "Failed to parse port '" << statement->tokens_[1]
                   << "': " << e.what();
        return -1;
      }
    }
    // Check if the statement has a child block
    // and recursively check for "root" directive in the child block
    else if (statement->child_block_.get() != nullptr) {
      int port = statement->child_block_->get_port();
      if (port != -1) {
        return port;
      }
    }
  }
  LOG(debug) << "No root directive found; defaulting to -1";
  return -1;  // Default value if no port is found
}

NginxLocationResult NginxConfig::get_locations() const {
  NginxLocationResult result;
  std::vector<NginxLocation> locations;

  for (const auto& statement : statements_) {
    if (statement->tokens_.size() == 3 && statement->tokens_[0] == "location") {
      NginxLocation location;
      location.path = statement->tokens_[1];
      location.handler = statement->tokens_[2];

      // error if path is quoted
      if (location.path.front() == '"' && location.path.back() == '"' ||
          location.path.front() == '\'' && location.path.back() == '\'') {
        LOG(error) << "Location path cannot be quoted";
        // return error return type
        result.valid = false;
        return result;
      }

      // error if path has trailing slash
      if (location.path.back() == '/') {
        LOG(error) << "Location path cannot have trailing slash";
        // return error return type
        result.valid = false;
        return result;
      }

      // error if path not starting with /
      if (location.path[0] != '/') {
        LOG(error) << "Location path must start with /";
        // return error return type
        result.valid = false;
        return result;
      }

      // error if path already exists
      for (const auto& existing_location : locations) {
        if (location.path.compare(existing_location.path) == 0) {
          LOG(error) << "Location path already exists";
          // return error return type
          result.valid = false;
          return result;
        }
      }

      // It is now coded statically
      // TODO: Make it dynamic
      if (location.handler == "StaticHandler") {
        if (statement->child_block_->statements_.size() == 1 &&
            statement->child_block_->statements_[0]->tokens_.size() == 2 &&
            statement->child_block_->statements_[0]->tokens_[0] == "root") {
          location.root = statement->child_block_->statements_[0]->tokens_[1];
          // error if root path has trailing slash

          // error if root path is quoted
          if (location.root.value().front() == '"' &&
                  location.root.value().back() == '"' ||
              location.root.value().front() == '\'' &&
                  location.root.value().back() == '\'') {
            LOG(error) << "Root path cannot be quoted";
            // return error return type
            result.valid = false;
            return result;
          }

          if (location.root.value().back() == '/') {
            LOG(error) << "Root path cannot have trailing slash";
            // return error return type
            result.valid = false;
            return result;
          }

          // normalize path if it is a relative path
          if (boost::filesystem::path(location.root.value()).is_relative()) {
            try {
              location.root.value() =
                  boost::filesystem::canonical(location.root.value()).string();
            } catch (const boost::filesystem::filesystem_error& e) {
              LOG(error) << "Root path does not exist: "
                         << location.root.value();
              result.valid = false;
              return result;
            }
          } else {
            try {
              // Check if absolute path exists and is accessible
              if (!boost::filesystem::exists(location.root.value())) {
                LOG(error) << "Root path does not exist: "
                           << location.root.value();
                result.valid = false;
                return result;
              }
              // Convert to canonical form to resolve any symlinks and normalize
              // the path
              location.root.value() =
                  boost::filesystem::canonical(location.root.value()).string();
            } catch (const boost::filesystem::filesystem_error& e) {
              LOG(error) << "Error accessing root path: "
                         << location.root.value() << " - " << e.what();
              result.valid = false;
              return result;
            }
          }

        } else {
          LOG(error) << "StaticHandler must have exactly one argument with "
                        "root directive";
          result.valid = false;
          return result;
        }
        locations.push_back(location);
        continue;
      }

      if (location.handler == "EchoHandler") {
        if (statement->child_block_->statements_.size() != 0) {
          LOG(error) << "EchoHandler must have no arguments";
          result.valid = false;
          return result;
        }
        locations.push_back(location);
        continue;
      }

      LOG(error) << "Location handler not found: " << location.handler;
      // return error return type
      result.valid = false;
      return result;
    } else if (statement->tokens_.size() >= 1 &&
               statement->tokens_[0] == "location") {
      LOG(error) << "Invalid location handler: " << statement->to_string(0);
      // return error return type
      result.valid = false;
      return result;
    }
  }

  result.valid = true;
  result.locations = locations;
  return result;
}

std::string NginxConfigStatement::to_string(int depth) {
  std::string serialized_statement;
  for (int i = 0; i < depth; ++i) {
    serialized_statement.append("  ");
  }
  for (unsigned int i = 0; i < tokens_.size(); ++i) {
    if (i != 0) {
      serialized_statement.append(" ");
    }
    serialized_statement.append(tokens_[i]);
  }
  if (child_block_.get() != nullptr) {
    serialized_statement.append(" {\n");
    serialized_statement.append(child_block_->to_string(depth + 1));
    for (int i = 0; i < depth; ++i) {
      serialized_statement.append("  ");
    }
    serialized_statement.append("}");
  } else {
    serialized_statement.append(";");
  }
  serialized_statement.append("\n");
  return serialized_statement;
}

NginxConfigParser::NginxConfigParser() {}

const char* NginxConfigParser::token_type_as_string(TokenType type) {
  switch (type) {
    case TOKEN_TYPE_START:
      return "TOKEN_TYPE_START";
    case TOKEN_TYPE_NORMAL:
      return "TOKEN_TYPE_NORMAL";
    case TOKEN_TYPE_START_BLOCK:
      return "TOKEN_TYPE_START_BLOCK";
    case TOKEN_TYPE_END_BLOCK:
      return "TOKEN_TYPE_END_BLOCK";
    case TOKEN_TYPE_COMMENT:
      return "TOKEN_TYPE_COMMENT";
    case TOKEN_TYPE_STATEMENT_END:
      return "TOKEN_TYPE_STATEMENT_END";
    case TOKEN_TYPE_EOF:
      return "TOKEN_TYPE_EOF";
    case TOKEN_TYPE_ERROR:
      return "TOKEN_TYPE_ERROR";
    default:
      return "Unknown token type";
  }
}

NginxConfigParser::TokenType NginxConfigParser::parse_token(
    std::istream* input, std::string* value) {
  TokenParserState state = TOKEN_STATE_INITIAL_WHITESPACE;
  while (input->good()) {
    const char c = input->get();
    if (!input->good()) {
      break;
    }
    switch (state) {
      case TOKEN_STATE_INITIAL_WHITESPACE:
        switch (c) {
          case '{':
            *value = c;
            return TOKEN_TYPE_START_BLOCK;
          case '}':
            *value = c;
            return TOKEN_TYPE_END_BLOCK;
          case '#':
            *value = c;
            state = TOKEN_STATE_TOKEN_TYPE_COMMENT;
            continue;
          case '"':
            *value = c;
            state = TOKEN_STATE_DOUBLE_QUOTE;
            continue;
          case '\'':
            *value = c;
            state = TOKEN_STATE_SINGLE_QUOTE;
            continue;
          case ';':
            *value = c;
            return TOKEN_TYPE_STATEMENT_END;
          case ' ':
          case '\t':
          case '\n':
          case '\r':
            continue;
          default:
            *value += c;
            state = TOKEN_STATE_TOKEN_TYPE_NORMAL;
            continue;
        }
      case TOKEN_STATE_SINGLE_QUOTE:
        // the end of a quoted token should not be followed by whitespace.
        // not allow for backslash-escaping within strings.
        *value += c;
        if (c == '\'') {
          return TOKEN_TYPE_NORMAL;
        }
        continue;
      case TOKEN_STATE_DOUBLE_QUOTE:
        *value += c;
        if (c == '"') {
          return TOKEN_TYPE_NORMAL;
        }
        continue;
      case TOKEN_STATE_TOKEN_TYPE_COMMENT:
        if (c == '\n' || c == '\r') {
          return TOKEN_TYPE_COMMENT;
        }
        *value += c;
        continue;
      case TOKEN_STATE_TOKEN_TYPE_NORMAL:
        if (c == ' ' || c == '\t' || c == '\n' || c == '\t' || c == ';' ||
            c == '{' || c == '}') {
          input->unget();
          return TOKEN_TYPE_NORMAL;
        }
        *value += c;
        continue;
    }
  }

  // If we get here, we reached the end of the file.
  if (state == TOKEN_STATE_SINGLE_QUOTE || state == TOKEN_STATE_DOUBLE_QUOTE) {
    LOG(error) << "Unterminated quote in config";
    return TOKEN_TYPE_ERROR;
  }

  LOG(debug) << "Reached EOF in parse_token";
  return TOKEN_TYPE_EOF;
}

bool NginxConfigParser::parse(std::istream* config_file, NginxConfig* config) {
  LOG(debug) << "Starting parse of nginx config";
  std::stack<NginxConfig*> config_stack;
  config_stack.push(config);
  TokenType last_token_type = TOKEN_TYPE_START;
  TokenType token_type;
  while (true) {
    std::string token;
    token_type = parse_token(config_file, &token);
    // printf ("%s: %s\n", token_type_as_string(token_type), token.c_str());
    if (token_type == TOKEN_TYPE_ERROR) {
      LOG(error) << "Parse error after token type "
                 << token_type_as_string(last_token_type);
      break;
    }

    if (token_type == TOKEN_TYPE_COMMENT) {
      // Skip comments.
      continue;
    }

    if (token_type == TOKEN_TYPE_NORMAL) {
      LOG(trace) << "Parsing NORMAL token: '" << token << "'";
      if (last_token_type == TOKEN_TYPE_START ||
          last_token_type == TOKEN_TYPE_STATEMENT_END ||
          last_token_type == TOKEN_TYPE_START_BLOCK ||
          last_token_type == TOKEN_TYPE_END_BLOCK ||
          last_token_type == TOKEN_TYPE_NORMAL) {
        if (last_token_type != TOKEN_TYPE_NORMAL) {
          config_stack.top()->statements_.emplace_back(
              new NginxConfigStatement);
        }
        config_stack.top()->statements_.back().get()->tokens_.push_back(token);
      } else {
        LOG(error) << "Unexpected NORMAL after "
                   << token_type_as_string(last_token_type);
        break;
      }
    } else if (token_type == TOKEN_TYPE_STATEMENT_END) {
      if (last_token_type != TOKEN_TYPE_NORMAL) {
        LOG(error) << "Unexpected STATEMENT_END after "
                   << token_type_as_string(last_token_type);
        break;
      }
      LOG(debug) << "Completed statement when encountering ';'";
    } else if (token_type == TOKEN_TYPE_START_BLOCK) {
      if (last_token_type != TOKEN_TYPE_NORMAL) {
        LOG(error) << "Unexpected START_BLOCK after "
                   << token_type_as_string(last_token_type);
        break;
      }
      NginxConfig* const new_config = new NginxConfig;
      config_stack.top()->statements_.back().get()->child_block_.reset(
          new_config);
      config_stack.push(new_config);
    } else if (token_type == TOKEN_TYPE_END_BLOCK) {
      if (last_token_type != TOKEN_TYPE_STATEMENT_END &&
          last_token_type != TOKEN_TYPE_END_BLOCK &&
          last_token_type != TOKEN_TYPE_START_BLOCK) {
        // handle cases }} and {}
        LOG(error) << "Unexpected END_BLOCK after "
                   << token_type_as_string(last_token_type);
        break;
      }
      config_stack.pop();
    } else if (token_type == TOKEN_TYPE_EOF) {
      if (last_token_type != TOKEN_TYPE_STATEMENT_END &&
          last_token_type != TOKEN_TYPE_END_BLOCK) {
        LOG(error) << "EOF in invalid state: last="
                   << token_type_as_string(last_token_type)
                   << ", stack_size=" << config_stack.size();
        break;
      } else if (config_stack.size() != 1) {
        LOG(error) << "EOF with unclosed blocks: last="
                   << token_type_as_string(last_token_type)
                   << ", stack_size=" << config_stack.size();
        break;
      }
      LOG(info) << "Finished parsing nginx config successfully";
      return true;
    } else {
      LOG(error) << "Unexpected token type: "
                 << token_type_as_string(token_type);
      break;
    }
    last_token_type = token_type;
  }

  LOG(error) << "Parsing failed; bad transition from "
             << token_type_as_string(last_token_type) << " to "
             << token_type_as_string(token_type);
  return false;
}

bool NginxConfigParser::parse(const char* file_name, NginxConfig* config) {
  LOG(debug) << "Opening config file: " << file_name;
  std::ifstream config_file;
  config_file.open(file_name);
  if (!config_file.good()) {
    LOG(error) << "Failed to open config file: " << file_name;
    return false;
  }

  const bool return_value =
      parse(dynamic_cast<std::istream*>(&config_file), config);
  config_file.close();
  return return_value;
}
