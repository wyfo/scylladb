/*
 * Copyright (C) 2015-present ScyllaDB
 *
 * Modified by ScyllaDB
 */

/*
 * SPDX-License-Identifier: (AGPL-3.0-or-later and Apache-2.0)
 */

#pragma once

#include "cql3/error_listener.hh"
#include "exceptions/exceptions.hh"
#include "types.hh"

namespace cql3 {

/**
 * <code>ErrorListener</code> that collect and enhance the errors send by the CQL lexer and parser.
 */
template<typename RecognizerType, typename TokenType, typename ExceptionBaseType>
class error_collector : public error_listener<RecognizerType, ExceptionBaseType> {
    /**
     * The offset of the first token of the snippet.
     */
    static constexpr int32_t FIRST_TOKEN_OFFSET = 10;

    /**
     * The offset of the last token of the snippet.
     */
    static constexpr int32_t LAST_TOKEN_OFFSET = 2;

    /**
     * The CQL query.
     */
    const sstring_view _query;

    /**
     * An empty bitset to be used as a workaround for AntLR null dereference
     * bug.
     */
    static typename ExceptionBaseType::BitsetListType _empty_bit_list;

public:

    /**
     * Creates a new <code>ErrorCollector</code> instance to collect the syntax errors associated to the specified CQL
     * query.
     *
     * @param query the CQL query that will be parsed
     */
    error_collector(const sstring_view& query) : _query(query) {}

    /**
     * Format and throw a new \c exceptions::syntax_exception.
     */
    [[noreturn]] virtual void syntax_error(RecognizerType& recognizer, ANTLR_UINT8** token_names, ExceptionBaseType* ex) override {
        auto hdr = get_error_header(ex);
        auto msg = get_error_message(recognizer, ex, token_names);
        std::stringstream result;
        result << hdr << ' ' << msg;
#if 0
        if (recognizer instanceof Parser)
            appendQuerySnippet((Parser) recognizer, builder);
#endif

        throw exceptions::syntax_exception(result.str());
    }

    /**
     * Throw a new \c exceptions::syntax_exception.
     */
    [[noreturn]] virtual void syntax_error(RecognizerType&, const sstring& msg) override {
        throw exceptions::syntax_exception(msg);
    }

private:
    std::string get_error_header(ExceptionBaseType* ex) {
        std::stringstream result;
        result << "line " << ex->get_line() << ":" << ex->get_charPositionInLine();
        return result.str();
    }

    std::string get_error_message(RecognizerType& recognizer, ExceptionBaseType* ex, ANTLR_UINT8** token_names)
    {
        using namespace antlr3;
        std::stringstream msg;
        switch (ex->getType()) {
        case ExceptionType::UNWANTED_TOKEN_EXCEPTION: {
            msg << "extraneous input " << get_token_error_display(recognizer, ex->get_token());
            if (token_names != nullptr) {
                std::string token_name;
                if (recognizer.is_eof_token(ex->get_expecting())) {
                    token_name = "EOF";
                } else {
                    token_name = reinterpret_cast<const char*>(token_names[ex->get_expecting()]);
                }
                msg << " expecting " << token_name;
            }
            break;
        }
        case ExceptionType::MISSING_TOKEN_EXCEPTION: {
            std::string token_name;
            if (token_names == nullptr) {
                token_name = "(" + std::to_string(ex->get_expecting()) + ")";
            } else {
                if (recognizer.is_eof_token(ex->get_expecting())) {
                    token_name = "EOF";
                } else {
                    token_name = reinterpret_cast<const char*>(token_names[ex->get_expecting()]);
                }
            }
            msg << "missing " << token_name << " at " << get_token_error_display(recognizer, ex->get_token());
            break;
        }
        case ExceptionType::NO_VIABLE_ALT_EXCEPTION: {
            msg << "no viable alternative at input " << get_token_error_display(recognizer, ex->get_token());
            break;
        }
        default:
            // AntLR Exception class has a bug of dereferencing a null
            // pointer in the displayRecognitionError. The following
            // if statement makes sure it will not be null before the
            // call to that function (displayRecognitionError).
            // bug reference: https://github.com/antlr/antlr3/issues/191
            if (!ex->get_expectingSet()) {
                ex->set_expectingSet(&_empty_bit_list);
            }
            ex->displayRecognitionError(token_names, msg);
        }
        return msg.str();
    }

    std::string get_token_error_display(RecognizerType& recognizer, const TokenType* token)
    {
        return "'" + recognizer.token_text(token) + "'";
    }

#if 0

    /**
     * Appends a query snippet to the message to help the user to understand the problem.
     *
     * @param parser the parser used to parse the query
     * @param builder the <code>StringBuilder</code> used to build the error message
     */
    private void appendQuerySnippet(Parser parser, StringBuilder builder)
    {
        TokenStream tokenStream = parser.getTokenStream();
        int index = tokenStream.index();
        int size = tokenStream.size();

        Token from = tokenStream.get(getSnippetFirstTokenIndex(index));
        Token to = tokenStream.get(getSnippetLastTokenIndex(index, size));
        Token offending = tokenStream.get(getOffendingTokenIndex(index, size));

        appendSnippet(builder, from, to, offending);
    }

    /**
     * Appends a query snippet to the message to help the user to understand the problem.
     *
     * @param from the first token to include within the snippet
     * @param to the last token to include within the snippet
     * @param offending the token which is responsible for the error
     */
    final void appendSnippet(StringBuilder builder,
                             Token from,
                             Token to,
                             Token offending)
    {
        if (!areTokensValid(from, to, offending))
            return;

        String[] lines = query.split("\n");

        boolean includeQueryStart = (from.getLine() == 1) && (from.getCharPositionInLine() == 0);
        boolean includeQueryEnd = (to.getLine() == lines.length)
                && (getLastCharPositionInLine(to) == lines[lines.length - 1].length());

        builder.append(" (");

        if (!includeQueryStart)
            builder.append("...");

        String toLine = lines[lineIndex(to)];
        int toEnd = getLastCharPositionInLine(to);
        lines[lineIndex(to)] = toEnd >= toLine.length() ? toLine : toLine.substring(0, toEnd);
        lines[lineIndex(offending)] = highlightToken(lines[lineIndex(offending)], offending);
        lines[lineIndex(from)] = lines[lineIndex(from)].substring(from.getCharPositionInLine());

        for (int i = lineIndex(from), m = lineIndex(to); i <= m; i++)
            builder.append(lines[i]);

        if (!includeQueryEnd)
            builder.append("...");

        builder.append(")");
    }

    /**
     * Checks if the specified tokens are valid.
     *
     * @param tokens the tokens to check
     * @return <code>true</code> if all the specified tokens are valid ones,
     * <code>false</code> otherwise.
     */
    private static boolean areTokensValid(Token... tokens)
    {
        for (Token token : tokens)
        {
            if (!isTokenValid(token))
                return false;
        }
        return true;
    }

    /**
     * Checks that the specified token is valid.
     *
     * @param token the token to check
     * @return <code>true</code> if it is considered as valid, <code>false</code> otherwise.
     */
    private static boolean isTokenValid(Token token)
    {
        return token.getLine() > 0 && token.getCharPositionInLine() >= 0;
    }

    /**
     * Returns the index of the offending token. <p>In the case where the offending token is an extra
     * character at the end, the index returned by the <code>TokenStream</code> might be after the last token.
     * To avoid that problem we need to make sure that the index of the offending token is a valid index 
     * (one for which a token exist).</p>
     *
     * @param index the token index returned by the <code>TokenStream</code>
     * @param size the <code>TokenStream</code> size
     * @return the valid index of the offending token
     */
    private static int getOffendingTokenIndex(int index, int size)
    {
        return Math.min(index, size - 1);
    }

    /**
     * Puts the specified token within square brackets.
     *
     * @param line the line containing the token
     * @param token the token to put within square brackets
     */
    private static String highlightToken(String line, Token token)
    {
        String newLine = insertChar(line, getLastCharPositionInLine(token), ']');
        return insertChar(newLine, token.getCharPositionInLine(), '[');
    }

    /**
     * Returns the index of the last character relative to the beginning of the line 0..n-1
     *
     * @param token the token
     * @return the index of the last character relative to the beginning of the line 0..n-1
     */
    private static int getLastCharPositionInLine(Token token)
    {
        return token.getCharPositionInLine() + getLength(token);
    }

    /**
     * Return the token length.
     *
     * @param token the token
     * @return the token length
     */
    private static int getLength(Token token)
    {
        return token.getText().length();
    }

    /**
     * Inserts a character at a given position within a <code>String</code>.
     *
     * @param s the <code>String</code> in which the character must be inserted
     * @param index the position where the character must be inserted
     * @param c the character to insert
     * @return the modified <code>String</code>
     */
    private static String insertChar(String s, int index, char c)
    {
        return new StringBuilder().append(s.substring(0, index))
                .append(c)
                .append(s.substring(index))
                .toString();
    }

    /**
     * Returns the index of the line number on which this token was matched; index=0..n-1
     *
     * @param token the token
     * @return the index of the line number on which this token was matched; index=0..n-1
     */
    private static int lineIndex(Token token)
    {
        return token.getLine() - 1;
    }

    /**
     * Returns the index of the last token which is part of the snippet.
     *
     * @param index the index of the token causing the error
     * @param size the total number of tokens
     * @return the index of the last token which is part of the snippet.
     */
    private static int getSnippetLastTokenIndex(int index, int size)
    {
        return Math.min(size - 1, index + LAST_TOKEN_OFFSET);
    }

    /**
     * Returns the index of the first token which is part of the snippet.
     *
     * @param index the index of the token causing the error
     * @return the index of the first token which is part of the snippet.
     */
    private static int getSnippetFirstTokenIndex(int index)
    {
        return Math.max(0, index - FIRST_TOKEN_OFFSET);
    }
#endif
};

template<typename RecognizerType, typename TokenType, typename ExceptionBaseType>
typename ExceptionBaseType::BitsetListType
error_collector<RecognizerType,TokenType,ExceptionBaseType>::_empty_bit_list = typename ExceptionBaseType::BitsetListType();

}
