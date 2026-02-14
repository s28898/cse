//
// Created by Bruno Kuś on 02/02/2026.
//

#ifndef MONGO_2_TODOERROR_H
#define MONGO_2_TODOERROR_H

#include <exception>
#include <source_location>
#include <string>
#include <sstream>
struct TodoError : std::exception
{
    std::string message;
    explicit TodoError(const std::source_location& location = std::source_location::current()) {
      std::stringstream ss;
      ss << location.file_name() << ":" << location.line()
      << ": " << "TODO";
      this->message = ss.str();
    }
    [[nodiscard]] const char * what() const noexcept override {
      return message.data();
    }
};


#endif //MONGO_2_TODOERROR_H
