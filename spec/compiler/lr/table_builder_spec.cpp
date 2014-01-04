#include "spec_helper.h"
#include <functional>
#include "table_builder.h"

using namespace tree_sitter::lr;
using namespace tree_sitter::rules;

typedef std::unordered_set<ParseAction> parse_actions;
typedef std::unordered_set<LexAction> lex_actions;

START_TEST

describe("building parse and lex tables", []() {
    Grammar grammar({
        { "expression", choice({
            seq({
                sym("term"),
                sym("plus-token"),
                sym("term") }),
            sym("term") }) },
        { "term", choice({
            sym("variable"),
            sym("number"),
            seq({
                sym("left-paren-token"),
                sym("expression"),
                sym("right-paren-token")
            }) }) },
        { "variable", sym("variable-token") },
        { "number", sym("number-token") }
    });
    
    Grammar lex_grammar({
        { "plus-token", character('+') },
        { "variable-token", pattern("\\w+") },
        { "number-token", pattern("\\d+") },
        { "left-paren-token", character('(') },
        { "right-paren-token", character(')') }
    });

    pair<ParseTable, LexTable> tables = build_tables(grammar, lex_grammar);
    ParseTable table = tables.first;
    LexTable lex_table = tables.second;
    
    function<ParseState(size_t)> parse_state = [&](size_t index) {
        return table.states[index];
    };
    
    function<LexState(size_t)> lex_state = [&](size_t parse_state_index) {
        size_t index = table.states[parse_state_index].lex_state_index;
        return lex_table.states[index];
    };
    
    it("has the right starting state", [&]() {
        AssertThat(parse_state(0).actions, Equals(unordered_map<string, parse_actions>({
            { "expression", parse_actions({ ParseAction::Shift(1) }) },
            { "term", parse_actions({ ParseAction::Shift(2) }) },
            { "number", parse_actions({ ParseAction::Shift(5) }) },
            { "variable", parse_actions({ ParseAction::Shift(5) }) },

            { "left-paren-token", parse_actions({ ParseAction::Shift(6) }) },
            { "variable-token", parse_actions({ ParseAction::Shift(9) }) },
            { "number-token", parse_actions({ ParseAction::Shift(10) }) },
        })));
        
        AssertThat(lex_state(0).actions, Equals(unordered_map<CharMatch, lex_actions>({
            { CharMatchSpecific('('), lex_actions({ LexAction::Advance(1) }) },
            { CharMatchClass(CharClassDigit), lex_actions({ LexAction::Advance(2) }) },
            { CharMatchClass(CharClassWord), lex_actions({ LexAction::Advance(3) }) },
        })));
    });
    
    it("accepts when the start symbol is reduced", [&]() {
        AssertThat(parse_state(1).actions, Equals(unordered_map<string, parse_actions>({
            { ParseTable::END_OF_INPUT, parse_actions({ ParseAction::Accept() }) }
        })));
    });
    
    it("has the right next states", [&]() {
        AssertThat(parse_state(2).actions, Equals(unordered_map<string, parse_actions>({
            { "plus-token", parse_actions({ ParseAction::Shift(3) }) },
        })));
    });
});

END_TEST