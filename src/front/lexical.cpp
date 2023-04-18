#include"front/lexical.h"

#include<map>
#include<cassert>
#include<string>

#define TODO assert(0 && "todo")
#define DEBUG_DFA

// #define DEBUG_DFA
// #define DEBUG_SCANNER

std::string frontend::toString(State s) {
    switch (s) {
    case State::Empty: return "Empty";
    case State::Ident: return "Ident";
    case State::IntLiteral: return "IntLiteral";
    case State::FloatLiteral: return "FloatLiteral";
    case State::op: return "op";
    default:
        assert(0 && "invalid State");
    }
    return "";
}

std::set<std::string> frontend::keywords= {
    "const", "int", "float", "if", "else", "while", "continue", "break", "return", "void"
};

frontend::DFA::DFA(): cur_state(frontend::State::Empty), cur_str() {}

frontend::DFA::~DFA() {}

bool isIdent(char c){
    return (c>='a' && c<='z') || (c>='A' && c<='Z');
}

bool isOp(char c){
    std::string s="";
    s+=c;
    return frontend::op_tokenType_m.count(s);
}

bool isNumber(char c){
    return c>='0' && c<='9';
}

bool isEmpty(char c){
    return c=='\n' || c=='\r' || c=='\40';
}


bool frontend::DFA::next(char input, Token& buf) {
#ifdef DEBUG_DFA
#include<iostream>
    std::cout << "in state [" << toString(cur_state) << "], input = \'" << input << "\', str = " << cur_str << "\n";
#endif
    switch (cur_state)
    {
    case State::Empty:
        cur_str=input;
        cur_state=isIdent(input)?State::Ident
        :isOp(input)?State::op
        :isEmpty(input)?State::Empty
        :input=='.'?State::FloatLiteral
        :State::IntLiteral;
        return false;
    case State::Ident:
        if(isEmpty(input)){
            buf.value=cur_str;
            buf.type=keyword_tokenType_m.count(cur_str)?keyword_tokenType_m[cur_str]:TokenType::IDENFR;
            reset();
            return true;
        }else if(isOp(input)){
            buf.value=cur_str;
            buf.type=keyword_tokenType_m.count(cur_str)?keyword_tokenType_m[cur_str]:TokenType::IDENFR;
            reset();
            cur_str=input;
            cur_state=State::op;
            return true;
        }
        else{
            cur_str+=input;
            return false;
        }
    
    case State::IntLiteral:
        if(!isNumber(input)){
            buf.value=cur_str;
            buf.type=TokenType::INTLTR;
            cur_str=input;
            cur_state=isIdent(input)?State::Ident
             :isOp(input)?State::op
             :isEmpty(input)?State::Empty
             :input=='.'?State::FloatLiteral
             :State::IntLiteral;
            return true;
        }else if(input=='.'){
            cur_state=State::FloatLiteral;
            cur_str+=input;
        }
        else{
            cur_str+=input;
        }
        return false;
    
    case State::FloatLiteral:
         if(isEmpty(input)){
            buf.value=cur_str;
            buf.type=TokenType::FLOATLTR;
            reset();
            return true;
        }else{
          return false;
        }
    TODO;
    case State::op:
        if(!op_tokenType_m.count(cur_str+input)){
            buf.type=op_tokenType_m[cur_str];
            buf.value=cur_str;
             cur_str=input;
             cur_state=isIdent(input)?State::Ident
             :isOp(input)?State::op
             :isEmpty(input)?State::Empty
             :input=='.'?State::FloatLiteral
             :State::IntLiteral;
            return true;
        }else{
           cur_str+=input;
        }
    default:
        assert(0 && "cannot find next stage:DFA, lexical");
        break;
    }
#ifdef DEBUG_DFA
    std::cout << "next state is [" << toString(cur_state) << "], next str = " << cur_str << '\n';
#endif
}

void frontend::DFA::reset() {
    cur_state = State::Empty;
    cur_str = "";
}

frontend::Scanner::Scanner(std::string filename): fin(filename) {
    if(!fin.is_open()) {
        assert(0 && "in Scanner constructor, input file cannot open");
    }
}

frontend::Scanner::~Scanner() {
    fin.close();
}

std::string preprocess(std::ifstream& fin) {
    std::string line;
    std::string result;
    bool in_comment = false;

    while (std::getline(fin, line)) {
        for (size_t i = 0; i < line.length(); ++i) {
            if (!in_comment && line[i] == '/' && i + 1 < line.length() && line[i+1] == '/') {
                break;
            } else if (!in_comment && line[i] == '/' && i + 1 < line.length() && line[i+1] == '*') {
                in_comment = true;
                ++i; 
            } else if (in_comment && line[i] == '*' && i + 1 < line.length() && line[i+1] == '/') {
                in_comment = false;
                ++i; 
            } else if (!in_comment) {
                result.push_back(line[i]);
            }
        }
        result.push_back('\n'); 
    }

    return result;
}

std::vector<frontend::Token> frontend::Scanner::run() {
    std::vector<Token> ret;
    Token tk;
    DFA dfa;
    std::string s = preprocess(fin);    // delete comments
    for(auto c: s) {
            if(dfa.next(c, tk)){
                ret.push_back(tk);
            }
    }
    if(!fin.eof()){
        assert(0 && "EOF error");
    }
#ifdef DEBUG_SCANNER
#include<iostream>
            std::cout << "token: " << toString(tk.type) << "\t" << tk.value << std::endl;
#endif
    return ret;
}