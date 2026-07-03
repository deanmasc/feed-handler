#ifndef PARSER_HEADER
#define PARSER_HEADER

#include <memory>
#include "../order_book/book_manager.h"

void process_message(char* msg, uint16_t len, BookManager& book_manager);

#endif