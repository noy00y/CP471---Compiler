/* Imports */ 
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cctype> 
#include <queue>
#include <array>
#include <map>
#include <algorithm>
#include <utility>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
using namespace std;

/* Constants and Global Declarations: */ 
#define MAX_KEYWORDS 100
#define BUFFER_SIZE 2048

// Lexical Analysis:
ifstream inputFile;
ofstream tokenFile;
ofstream errorFile;

vector<string> keywords(34); // eg. for, do, while, etc...
array<array<int, 127>, 30> table{}; // transition table for automaton machine (30 states, 127 inputs)
vector<char> buffer1(BUFFER_SIZE); // Dual buffers for reading 
vector<char> buffer2(BUFFER_SIZE); 
vector<char>* currentBuffer = &buffer1;

int character = 0; 
int line = 0; // track line while parsing

// Syntax Analysis
ifstream lexemmeFile;
string tokenVal; // for parsing soruce file
string tokenType;
using ProductionRule = vector<string>; // grammer production rule
using LL1Key = pair<string, string>; // pair of current non terminal and lookahead terminal
using LL1table = map<LL1Key, ProductionRule>; // sparse ll1 table
LL1table ll1table;

// Semantic Analysis
string scope = "global";

// Intermediate Code Gen
ofstream ICGFile; // output file for intermediate code
// int tempNum = 1; // t1, t2, t3, etc...
// int labelNum = 1; // lab1, lab2, etc...
// int loopNum = 1; // loop1, loop2, etc..

/* Structs and Enums*/
enum TokenType {
    // General
    T_IDENTIFIER,
    T_LITERAL,
    T_EOF, // End of file
    T_ERROR, // Error

    // Numbers
    T_INT,
    T_DOUBLE,

    // RELOP Operators (Keywords)
    K_EQL, // =
    K_PLUS, // +
    K_MINUS, // -
    K_MULTIPY, // *
    K_DIVIDE, // /
    K_MOD, // %

    // Comparison Operators (Keyword)
    K_LS_EQL, // <=
    K_NOT_EQL, // <>
    K_LS_THEN, // <
    K_EQL_TO, // ==
    K_GR_EQL, // >=
    K_GT_THEN, // >

    // Other Specific Keywords
    K_INT, // int keyword
    K_DOUBLE, // double keyword
    K_LPAREN, // (
    K_RPAREN, // )
    K_LBRACKET, // [
    K_RBRACKET, // ]
    K_DEF,
    K_FED,
    K_SEMI_COL, // ;
    K_COMMA, // ,
    K_DOT, // .
    K_IF,
    K_THEN,
    K_WHILE,
    K_DO,
    K_OD,
    K_PRINT,
    K_RETURN,
    K_FI,
    K_ELSE,
    K_OR,
    K_AND,
    K_NOT
};

string tokenTypeToString(TokenType type) {
    switch (type) {
        case T_IDENTIFIER: return "T_IDENTIFIER";
        case T_LITERAL: return "T_LITERAL";
        case T_EOF: return "T_EOF";
        case T_ERROR: return "T_ERROR";
        case T_INT: return "T_INT";
        case T_DOUBLE: return "T_DOUBLE";
        case K_EQL: return "K_EQL";
        case K_PLUS: return "K_PLUS";
        case K_MINUS: return "K_MINUS";
        case K_MULTIPY: return "K_MULTIPY";
        case K_DIVIDE: return "K_DIVIDE";
        case K_MOD: return "K_MOD";
        case K_LS_EQL: return "K_LS_EQL";
        case K_NOT_EQL: return "K_NOT_EQL";
        case K_LS_THEN: return "K_LS_THEN";
        case K_EQL_TO: return "K_EQL_TO";
        case K_GR_EQL: return "K_GR_EQL";
        case K_GT_THEN: return "K_GT_THEN";
        case K_INT: return "K_INT"; // Might conflict with T_INT
        case K_DOUBLE: return "K_DOUBLE"; // Might conflict with T_DOUBLE
        case K_LPAREN: return "K_LPAREN";
        case K_RPAREN: return "K_RPAREN";
        case K_LBRACKET: return "K_LBRACKET";
        case K_RBRACKET: return "K_RBRACKET";
        case K_DEF: return "K_DEF";
        case K_FED: return "K_FED";
        case K_SEMI_COL: return "K_SEMI_COL";
        case K_COMMA: return "K_COMMA";
        case K_DOT: return "K_DOT";
        case K_IF: return "K_IF";
        case K_THEN: return "K_THEN";
        case K_WHILE: return "K_WHILE";
        case K_DO: return "K_DO";
        case K_OD: return "K_OD";
        case K_PRINT: return "K_PRINT";
        case K_RETURN: return "K_RETURN";
        case K_FI: return "K_FI";
        case K_ELSE: return "K_ELSE";
        case K_OR: return "K_OR";
        case K_AND: return "K_AND";
        case K_NOT: return "K_NOT";
        default: return "Unknown TokenType";
    }
}

struct SymbolEntry {
    string type; // K_DEF, K_IF, K_WHILE, K_INT, K_DOUBLE
    shared_ptr<class SymbolTable> childTable; // child table for new scope (functions, if or while)
    string varName; // symbol name for K_DEF, or K_INT/K_DOUBLE

    // Specific to K_IF:
    union {int intVal; double doubleVal;}; // for int or double var declarations (eg. int x = 4;)
    
    // Specific to K_DEF
    string returnType; // K_INT or K_DOUBLE
    vector<pair<string, string>> params; // function params (type, var)
};

/* Classes */
class SymbolTable {
public:
    string scopeName;
    map<string, SymbolEntry> table;
    shared_ptr<SymbolTable> parentTable;

    SymbolTable(const string& name, shared_ptr<SymbolTable> parent = nullptr)
        : scopeName(name), parentTable(parent) {}

    void addEntry(const string& name, const SymbolEntry& entry) {
        table[name] = entry;
    }

    // Function to find an entry in the current table or in any parent table.
    optional<SymbolEntry> findEntry(const string& name) {
        auto it = table.find(name);
        if (it != table.end()) {
            return it->second;
        } else if (parentTable) {
            return parentTable->findEntry(name);
        }
        return nullopt;  
    }
};

class ASTNode : public enable_shared_from_this<ASTNode> {
public:
    string nodeType; // Identifies the type of the node
    string value; // Optional: For terminals or specific values
    vector<shared_ptr<ASTNode>> children; // Child nodes
    weak_ptr<ASTNode> parent; // Pointer to parent node

    // Generic Constructor and Deconstructor
    ASTNode(const string& type = "", const string& val = "") : nodeType(type), value(val) {}
    virtual ~ASTNode() = default;

    // Add child node for recursive decent and link its parent node
    void addChild(shared_ptr<ASTNode> child) {
        children.push_back(child);
        child->parent = shared_from_this();
    }

    // Returns the type of the node. Can be overridden by subclasses if needed.
    virtual string typeName() const { return nodeType; }
};

class Token {
public:
    TokenType type; 
    vector<char> buffer;
    int line; // line and char where token starts
    int character;

    // Constructor to initialize the Token
    Token() : line(0), character(0) {
        buffer.reserve(BUFFER_SIZE); // Pre-allocate memory for efficiency
    }

    // Class Functions
    bool isBlank() const {
        for (char ch : buffer) {
            if (!isspace(static_cast<unsigned char>(ch)))
                return false;
        }
        return true;
    }

    void determineType(const vector<string>& keywords) {
        if (!isBlank()) {
            string tokenStr(buffer.begin(), buffer.end());

            auto it = find_if(keywords.begin(), keywords.end(),
                                   [&tokenStr](const string& keyword) {
                                       return tokenStr == keyword;
                                   });

            if (it != keywords.end()) {
                // Mapping keywords to TokenType
                if (*it == "def") type = TokenType::K_DEF;
                else if (*it == "int") type = TokenType::K_INT;
                else if (*it == "double") type = TokenType::K_DOUBLE;
                else if (*it == "if") type = TokenType::K_IF;
                else if (*it == "then") type = TokenType::K_THEN;
                else if (*it == "fed") type = TokenType::K_FED;
                else if (*it == "fi") type = TokenType::K_FI;
                else if (*it == "else") type = TokenType::K_ELSE;
                else if (*it == "while") type = TokenType::K_WHILE;
                else if (*it == "print") type = TokenType::K_PRINT;
                else if (*it == "return") type = TokenType::K_RETURN;
                else if (*it == "or") type = TokenType::K_OR;
                else if (*it == "od") type = TokenType::K_OD;
                else if (*it == "and") type = TokenType::K_AND;
                else if (*it == "not") type = TokenType::K_NOT;
                else if (*it == "do") type = TokenType::K_DO;
            }
            else {
                // If no keyword matches, default to an identifier
                type = TokenType::T_IDENTIFIER;
            }
        }
    }
};

/* Functions */
// Parse source code for chars and return tokens
Token getNextToken() {
    Token token; // Token now uses its default constructor for initialization
    int currentState = 0;
    char currentChar;
    int character = 0; // track token positions

    while (inputFile.get(currentChar)) { 
        int ascii = static_cast<int>(currentChar); // Use static_cast for conversions in C++
        // cout << "Token: " << currentChar << " --> ascii: " << ascii << ", currentState = " << currentState << endl;
        /* Return current token given following cases
            - relop char --> states 1, 5 and 6 and current char is != to <, > or =
            - digit char --> states 13, 15 and 18 and current char is != to 0-9, . or E/e
            - a-z char --> states 10 and current char is != a-z
         */
        if ((currentState == 1 || currentState == 5 || currentState == 6) && (ascii < 60 || ascii > 62)) {
            // cout << "Return " << currentChar << " w/ ascii = " << ascii << " back to the file stream" << endl;
            inputFile.unget();

            // Determining Token Type:
            if (currentState == 1) token.type = TokenType::K_LS_THEN;
            else if (currentState == 5) token.type = TokenType::K_EQL;
            else if (currentState == 6) token.type = TokenType::K_GT_THEN;

            token.character = character;
            token.line = line;
            return token;
        }

        else if ((currentState == 13 || currentState == 15 || currentState == 18) && (ascii <  48 || ascii > 57) && (ascii != 46) && (ascii != 69)&& (ascii != 101)) {
            // cout << "Return " << currentChar << " w/ ascii = " << ascii << " back to the file stream" << endl;
            inputFile.unget();
            if (currentState == 13) token.type = TokenType::T_INT;
            else if (currentState == 15 || currentState == 18) token.type = TokenType::T_DOUBLE;
            token.character = character;
            token.line = line;
            return token;
        }

        else if ((currentState == 10) && (ascii < 97 || ascii > 122)) {
            // cout << "Return " << currentChar << " w/ ascii = " << ascii << " back to the file stream" << endl;
            inputFile.unget(); // Put character back into stream
            token.determineType(keywords);      
            token.character = character;   
            token.line = line;   
            return token;
        }

        /* Get current state */
        // If a-z --> set state = 10
        // If e and currentState = 15 --> set state = 16
        else if (ascii >= 97 && ascii <= 122) {
            if (currentState == 15 && ascii == 101) currentState = table[currentState][101];
            else currentState = table[currentState][97];
            // cout << "Token: " << currentChar << " w/ ascii: " << ascii << " is a-z --> currentState = " << currentState << endl;
        }
        
        // If ws or \n --> set state 100, 
        else if (ascii == 32 || ascii == 10) {
            if (ascii == 10) line += 1; // increment line count
            currentState = table[currentState][ascii];
            // cout << "Token: " << currentChar << " w/ ascii: " << ascii << " is ws --> currentState = " << currentState << endl;
        }

        // Double Logic:
        else if (ascii >= 48 && ascii <= 57) {
            currentState = table[currentState][48];
            // cout << "Token: " << currentChar << " w/ ascii: " << ascii << " is 0-9 --> currentState = " << currentState << endl;
        }

        // If E --> follow transition table with low e (accepted with double)
        else if (ascii == 69) {
            currentState = table[currentState][101];
            // cout << "Token: " << currentChar << " w/ ascii: " << ascii << " is E --> currentState = " << currentState << endl;
        }

        // If +, -, .  --> follow transition table (accepted with double)
        else if (ascii == 43 || ascii == 45 || ascii == 46) {
            currentState = table[currentState][ascii];
            if (currentState == 0) {
                token.buffer.push_back(currentChar);
                
                // Determine Type
                if (ascii == 43) token.type = TokenType::K_PLUS;
                else if (ascii == 45) token.type = TokenType::K_MINUS;
                else if (ascii == 46) token.type = TokenType::K_DOT;
                token.character = character;
                token.line = line;
                return token;
            }
            // cout << "Token: " << currentChar << " w/ ascii: " << ascii << " is +-. --> currentState = " << currentState << endl;
        }

        else if (ascii >= 60 && ascii <= 62) {
            currentState = table[currentState][ascii];
            // cout << "Token: " << currentChar << " w/ ascii: " << ascii << " is <,>,= --> currentState = " << currentState << endl;
        }
            
        // other special
        else {
            currentState = table[currentState][50];
            // cout << "Token: " << currentChar << " w/ ascii: " << ascii << " is other --> currentState = " << currentState << endl;
        }        

        /* Automaton Decisions*/
        // States 1, 6, 10, 13 --> add char to buffer
        if (currentState == 1  || currentState == 5 || currentState == 6 || currentState == 10 || currentState == 13 || currentState == 14 || currentState == 15 || currentState == 16 || currentState == 17 || currentState == 18) {
            token.buffer.push_back(currentChar);
        }

        // Accept single special keyword (add char to buffer)
        else if (currentState == 51) {
            token.buffer.push_back(currentChar);
        
            // Determine Token Type:
            if (ascii == 40) token.type = TokenType::K_LPAREN;
            else if (ascii == 41) token.type = TokenType::K_RPAREN;
            else if (ascii == 91) token.type = TokenType::K_LBRACKET;
            else if (ascii == 93) token.type = TokenType::K_RBRACKET;
            else if (ascii == 37) token.type = TokenType::K_MOD;
            else if (ascii == 61) token.type = TokenType::K_EQL;
            else if (ascii == 60) token.type = TokenType::K_LS_THEN;
            else if (ascii == 62) token.type = TokenType::K_GT_THEN;
            else if (ascii == 42) token.type = TokenType::K_MULTIPY;
            else if (ascii == 45) token.type = TokenType::K_MINUS;
            else if (ascii == 46) token.type = TokenType::K_DOT;
            else if (ascii == 43) token.type = TokenType::K_PLUS;
            else if (ascii == 59) token.type = TokenType::K_SEMI_COL;
            else if (ascii == 44) token.type = TokenType::K_COMMA;
            else if (ascii == 47) token.type = TokenType::K_DIVIDE;

            token.character = character;
            token.line = line;
            return token;
        }

        // Accept Operators --> 2, 3, 5, 7, 9 (add to buffer first)
        else if (currentState == 2 || currentState == 3 || currentState == 7 || currentState == 9) {
            token.buffer.push_back(currentChar);    

            // Determine Token Type:
            if (currentState == 2) token.type = TokenType::K_LS_EQL;
            else if (currentState == 3) token.type = TokenType::K_NOT_EQL;
            else if (currentState == 7) token.type = TokenType::K_GR_EQL;
            else if (currentState == 9) token.type = TokenType::K_EQL_TO;

            token.character = character;
            token.line = line;
            return token;
        }

        // Accept Single Operator --> 4, 8 --> Need to fix this because its not accepting the char that comes after
        else if (currentState == 4 || currentState == 8) {
            
            // Determine Token Type:
            if (currentState == 4) token.type = TokenType::K_GT_THEN;
            else if (currentState == 8) token.type = TokenType::K_LS_THEN;

            token.character = character;
            token.line = line;
            return token;
        }

        // Accept Identifer if at 100
        else if (currentState == 100) {
            token.determineType(keywords);     
            token.character = character;   
            token.line = line;
            return token;
        }

        character += 1;
    }
    token.type = TokenType::T_EOF;
    return token;
}

// Read each token from tokens.txt and update references. Once empty return $ token
void parseTokens() {
    string line;
    if (getline(lexemmeFile, line)) {
        istringstream iss(line.substr(1, line.size() - 2)); // Remove < and >
        string val, type;
        getline(iss, val, ',');
        getline(iss, type);
        type.erase(0, 1); // Remove leading space
        
        tokenVal = val;
        tokenType = type;
    }
    else {
        tokenVal = "";
        tokenType = "$"; // end of source file
    }
}

// Debugging
void printAST(const shared_ptr<ASTNode>& node, int level = 0) {
    if (!node) return; // Guard against null pointers

    // Print the current node with indentation based on its level in the tree
    cout << string(level * 2, ' ') << node->typeName(); // Indent based on level
    if (!node->value.empty()) {
        cout << " (Value: " << node->value << ")";
    } 
    cout << endl;

    // Recursively print each child
    for (const auto& child : node->children) {
        printAST(child, level + 1); // Increase level for child nodes
    }
}

// Recursively decend and match tokens:
void recursiveDecent(const string& currProd, shared_ptr<ASTNode> currentNode, shared_ptr<ASTNode> debugRoot) {
    if (tokenType == "$") return; // stop Decent if source file empty
    else if (tokenType == " K_COMMA") {
        tokenType = "K_COMMA";
        tokenVal = ",";
    }

    auto productions = ll1table[{currProd, tokenType}]; // Get production vector from <nonTerminal, tokenType> pair
    if (productions.empty()) {
        // printAST(debugRoot);
        errorFile << "Syntax Error: No production for " << currProd << " and " << tokenType << endl; // If blank production --> log error
        currentNode->value = "Syntax Error for --> " + tokenVal + " ";
        parseTokens();
        recursiveDecent(currProd, currentNode, debugRoot);
    }

    /**
     * Expand child productions recursively:
     * - Add each child prod to tree and link parent
     * - Case A: If ε prod --> return
     * - Case B: If prod matches tokenType --> update child astNode val, continue parsing next production of parent with updated tokenVal and tokenType
     * - Case C: Else --> recursive decent on current non terminal
    */
    for (const auto& p:productions) {
        // Create node with prod and add it as child to currentNode
        auto childProd = make_shared<ASTNode>(p);
        currentNode->addChild(childProd);

        // Cases
        if (p == "ε") 
            return;
        else if (p == tokenType) {
            childProd->value = tokenVal;
            parseTokens(); // "Consume" current token by updating address to tokenVal, tokenType to next token
            continue;
        }
        else recursiveDecent(p, childProd, debugRoot);
    }
}

// Generates Transition Table
void generateTable() {
    ifstream file("table.txt");
    if (!file) {
        cerr << "Error opening file" << endl;
        return; 
    }

    int state, input, next_state;
    char comma; 
    while (file >> state >> comma >> input >> comma >> next_state) {
        table[state][input] = next_state; 
    }
}

// Generates Reserved/Keyword Array
void loadKeywords() {
    ifstream file("keywords.txt");
    if (!file) {
        cerr << "Error opening file" << endl;
        return;
    }

    string keyword;
    while (getline(file, keyword)) {
        keywords.push_back(keyword);
    }
}

// Loads Ll1 table with ll1 grammer
void loadLL1() {

    // S' --> Start
    ll1table[{"S'", "K_SEMI_COL"}] = {"program", "$"};
    ll1table[{"S'", "K_DEF"}] = {"program", "$"};
    ll1table[{"S'", "K_INT"}] = {"program", "$"};
    ll1table[{"S'", "K_DOUBLE"}] = {"program", "$"};
    ll1table[{"S'", "K_IF"}] = {"program", "$"};
    ll1table[{"S'", "K_WHILE"}] = {"program", "$"};
    ll1table[{"S'", "K_PRINT"}] = {"program", "$"};
    ll1table[{"S'", "K_RETURN"}] = {"program", "$"};
    ll1table[{"S'", "T_IDENTIFIER"}] = {"program", "$"};

    // Program
    ll1table[{"program", "K_SEMI_COL"}] = {"fdecls", "declarations", "statement_seq", "K_DOT"};
    ll1table[{"program", "K_DEF"}] = {"fdecls", "declarations", "statement_seq", "K_DOT"};
    ll1table[{"program", "K_INT"}] = {"fdecls", "declarations", "statement_seq", "K_DOT"};
    ll1table[{"program", "K_DOUBLE"}] = {"fdecls", "declarations", "statement_seq", "K_DOT"};
    ll1table[{"program", "K_IF"}] = {"fdecls", "declarations", "statement_seq", "K_DOT"};
    ll1table[{"program", "K_WHILE"}] = {"fdecls", "declarations", "statement_seq", "K_DOT"};
    ll1table[{"program", "K_PRINT"}] = {"fdecls", "declarations", "statement_seq", "K_DOT"};
    ll1table[{"program", "K_RETURN"}] = {"fdecls", "declarations", "statement_seq", "K_DOT"};
    ll1table[{"program", "T_IDENTIFIER"}] = {"fdecls", "declarations", "statement_seq", "K_DOT"};

    // Function Declarations (Recursive):
    ll1table[{"fdecls", "K_SEMI_COL"}] = {"ε"};
    ll1table[{"fdecls", "K_DEF"}] = {"fdec", "K_SEMI_COL", "fdecls"};
    ll1table[{"fdecls", "K_INT"}] = {"ε"};
    ll1table[{"fdecls", "K_DOUBLE"}] = {"ε"};
    ll1table[{"fdecls", "K_IF"}] = {"ε"};
    ll1table[{"fdecls", "K_WHILE"}] = {"ε"};
    ll1table[{"fdecls", "K_PRINT"}] = {"ε"};
    ll1table[{"fdecls", "K_RETURN"}] = {"ε"};
    ll1table[{"fdecls", "T_IDENTIFIER"}] = {"ε"};

    // Function Declaration:
    ll1table[{"fdec", "K_DEF"}] = {"K_DEF", "type", "fname", "K_LPAREN", "params", "K_RPAREN", "declarations", "statement_seq", "K_FED"};

    // Parameters:
    ll1table[{"params", "K_INT"}] = {"type", "var", "paramsp"};
    ll1table[{"params", "K_DOUBLE"}] = {"type", "var", "paramsp"};

    // Parameters Prime:
    ll1table[{"paramsp", "K_RPAREN"}] = {"ε"};
    ll1table[{"paramsp", "K_COMMA"}] = {"K_COMMA", "params"};

    // Function Name:
    ll1table[{"fname", "T_IDENTIFIER"}] = {"id"};

    // Declarations (Recursive):
    ll1table[{"declarations", "K_SEMI_COL"}] = {"ε"};
    ll1table[{"declarations", "K_INT"}] = {"decl", "K_SEMI_COL", "declarations"};
    ll1table[{"declarations", "K_DOUBLE"}] = {"decl", "K_SEMI_COL", "declarations"};
    ll1table[{"declarations", "K_IF"}] = {"ε"};
    ll1table[{"declarations", "K_WHILE"}] = {"ε"};
    ll1table[{"declarations", "K_PRINT"}] = {"ε"};
    ll1table[{"declarations", "K_RETURN"}] = {"ε"};
    ll1table[{"declarations", "T_IDENTIFIER"}] = {"ε"};
    // modifications to og grammer
    // ll1table[{"declarations", "K_DEF"}] = {"K_DEF", "type", "fname", "K_LPAREN", "params", "K_RPAREN", "declarations", "statement_seq", "K_FED"}; 
    ll1table[{"declarations", "K_DEF"}] = {"fdecls"}; 
    
    // Declaration:
    ll1table[{"decl", "K_INT"}] = {"type", "varlist"};
    ll1table[{"decl", "K_DOUBLE"}] = {"type", "varlist"};

    // Type:
    ll1table[{"type", "K_INT"}] = {"K_INT"};
    ll1table[{"type", "K_DOUBLE"}] = {"K_DOUBLE"};

    // Variable List:
    ll1table[{"varlist", "K_SEMI_COL"}] = {"ε"};
    ll1table[{"varlist", "T_IDENTIFIER"}] = {"var", "varlistp"};

    // Variable List Prime:
    ll1table[{"varlistp", "K_SEMI_COL"}] = {"ε"};
    ll1table[{"varlistp", "K_COMMA"}] = {"K_COMMA", "varlist"};

    // Statement Sequence:
    ll1table[{"statement_seq", "K_SEMI_COL"}] = {"statement", "statement_seqp"};
    ll1table[{"statement_seq", "K_IF"}] = {"statement", "statement_seqp"};
    ll1table[{"statement_seq", "K_WHILE"}] = {"statement", "statement_seqp"};
    ll1table[{"statement_seq", "K_PRINT"}] = {"statement", "statement_seqp"};
    ll1table[{"statement_seq", "K_RETURN"}] = {"statement", "statement_seqp"};
    ll1table[{"statement_seq", "T_IDENTIFIER"}] = {"statement", "statement_seqp"};
    ll1table[{"statement_seq", "K_FED"}] = {"ε"}; // grammer modification
    ll1table[{"statement_seq", "K_OD"}] = {"ε"}; // modifications

    // Statement Sequence Prime:
    ll1table[{"statement_seqp", "K_SEMI_COL"}] = {"K_SEMI_COL", "statement_seq"};
    ll1table[{"statement_seqp", "K_FED"}] = {"ε"}; // grammer modification
    ll1table[{"statement_seqp", "K_OD"}] = {"ε"}; // grammer modification
    ll1table[{"statement_seqp", "K_FI"}] = {"ε"}; // grammer modification
    ll1table[{"statement_seqp", "K_ELSE"}] = {"ε"}; // grammer modification
    ll1table[{"statement_seqp", "K_MULTIPY"}] = {"K_MULTIPY", "factor", "termp"};

    // Statement:
    ll1table[{"statement", "K_IF"}] = {"K_IF", "bexpr", "K_THEN", "statement_seq", "statementp"};
    ll1table[{"statement", "K_WHILE"}] = {"K_WHILE", "bexpr", "K_DO", "statement_seq", "K_OD"};
    ll1table[{"statement", "K_PRINT"}] = {"K_PRINT", "expr"};
    ll1table[{"statement", "K_RETURN"}] = {"K_RETURN", "expr"};
    ll1table[{"statement", "T_IDENTIFIER"}] = {"var", "K_EQL", "expr"};
    ll1table[{"statement", "K_FED"}] = {"ε"}; // grammer modification

    // Statement Prime:
    ll1table[{"statementp", "K_FI"}] = {"K_FI"};
    ll1table[{"statementp", "K_ELSE"}] = {"K_ELSE", "statement_seq", "K_FI"};
    
    // Expression:
    ll1table[{"expr", "K_LPAREN"}] = {"term", "exprp"};
    ll1table[{"expr", "T_IDENTIFIER"}] = {"term", "exprp"};

    ll1table[{"expr", "K_FED"}] = {"ε"}; // grammer modification
    ll1table[{"expr", "K_OD"}] = {"ε"}; // grammer modification
    ll1table[{"expr", "T_INT"}] = {"T_INT"}; // grammer modification
    ll1table[{"expr", "T_DOUBLE"}] = {"T_DOUBLE"}; // grammer modification

    // Expression Prime:
    ll1table[{"exprp", "K_SEMI_COL"}] = {"ε"};
    ll1table[{"exprp", "K_RPAREN"}] = {"ε"};
    ll1table[{"exprp", "K_COMMA"}] = {"ε"};
    ll1table[{"exprp", "K_THEN"}] = {"ε"};
    ll1table[{"exprp", "K_DO"}] = {"ε"};
    ll1table[{"exprp", "K_PLUS"}] = {"K_PLUS", "term", "exprp"};
    ll1table[{"exprp", "K_MINUS"}] = {"K_MINUS", "term", "exprp"};
    ll1table[{"exprp", "K_OR"}] = {"ε"};
    ll1table[{"exprp", "K_AND"}] = {"ε"};
    ll1table[{"exprp", "K_LS_THEN"}] = {"ε"};
    ll1table[{"exprp", "K_GT_THEN"}] = {"ε"};
    ll1table[{"exprp", "K_EQL_TO"}] = {"ε"};
    ll1table[{"exprp", "K_LS_EQL"}] = {"ε"};
    ll1table[{"exprp", "K_GR_EQL"}] = {"ε"};
    ll1table[{"exprp", "K_NOT_EQL"}] = {"ε"};
    ll1table[{"exprp", "K_RBRACKET"}] = {"ε"};

    ll1table[{"exprp", "K_FED"}] = {"ε"}; // grammer modification
    ll1table[{"exprp", "K_OD"}] = {"ε"}; // grammer modification
    ll1table[{"exprp", "K_FI"}] = {"ε"}; // grammer modification
    ll1table[{"exprp", "K_ELSE"}] = {"ε"}; // grammer modification


    // Term:
    ll1table[{"term", "K_LPAREN"}] = {"factor", "termp"};
    ll1table[{"term", "T_IDENTIFIER"}] = {"factor", "termp"};
    ll1table[{"term", "K_FED"}] = {"ε"}; // grammer modification
    ll1table[{"term", "T_INT"}] = {"T_INT"}; // grammer modification
    ll1table[{"term", "T_DOUBLE"}] = {"T_DOUBLE"}; // grammer modification

    // Term Prime:
    ll1table[{"termp", "K_SEMI_COL"}] = {"ε"};
    ll1table[{"termp", "K_RPAREN"}] = {"ε"};
    ll1table[{"termp", "K_COMMA"}] = {"ε"};
    ll1table[{"termp", "K_THEN"}] = {"ε"};
    ll1table[{"termp", "K_DO"}] = {"ε"};
    ll1table[{"termp", "K_PLUS"}] = {"ε"};
    ll1table[{"termp", "K_MINUS"}] = {"ε"};
    ll1table[{"termp", "K_MULTIPY"}] = {"K_MULTIPY", "factor", "termp"};
    ll1table[{"termp", "K_DIVIDE"}] = {"K_DIVIDE", "factor", "termp"};
    ll1table[{"termp", "K_MOD"}] = {"K_MOD", "factor", "termp"};
    ll1table[{"termp", "K_OR"}] = {"ε"};
    ll1table[{"termp", "K_AND"}] = {"ε"};
    ll1table[{"termp", "K_LS_THEN"}] = {"ε"};
    ll1table[{"termp", "K_GT_THEN"}] = {"ε"};
    ll1table[{"termp", "K_EQL_TO"}] = {"ε"};
    ll1table[{"termp", "K_LS_EQL"}] = {"ε"};
    ll1table[{"termp", "K_GR_EQL"}] = {"ε"};
    ll1table[{"termp", "K_NOT_EQL"}] = {"ε"};
    ll1table[{"termp", "K_RBRACKET"}] = {"ε"};
    ll1table[{"termp", "K_FED"}] = {"ε"}; // grammer modification
    ll1table[{"termp", "K_FI"}] = {"ε"}; // grammer modification
    ll1table[{"termp", "K_ELSE"}] = {"ε"}; // grammer modification


    // Factor:
    ll1table[{"factor", "K_LPAREN"}] = {"K_LPAREN", "expr", "K_RPAREN"};
    // ll1table[{"factor", "T_IDENTIFIER"}] = {"id", "K_LPAREN", "exprseq", "K_RPAREN"}; 
    ll1table[{"factor", "T_IDENTIFIER"}] = {"id", "factorp"};
    ll1table[{"factor", "K_FED"}] = {"ε"}; // grammer modification
    ll1table[{"factor", "T_INT"}] = {"T_INT"}; // grammer modification
    ll1table[{"factor", "T_DOUBLE"}] = {"T_DOUBLE"}; // grammer modification


    // Factor Prime:
    ll1table[{"factorp", "K_LPAREN"}] = {"K_LPAREN", "exprseq", "K_RPAREN"};
    ll1table[{"factorp", "K_RPAREN"}] = {"ε"};
    ll1table[{"factorp", "K_COMMA"}] = {"ε"};
    ll1table[{"factorp", "K_THEN"}] = {"ε"};
    ll1table[{"factorp", "K_DO"}] = {"ε"};
    ll1table[{"factorp", "K_PLUS"}] = {"ε"};
    ll1table[{"factorp", "K_MINUS"}] = {"ε"};
    ll1table[{"factorp", "K_MULTIPY"}] = {"ε"};
    ll1table[{"factorp", "K_DIVIDE"}] = {"ε"};
    ll1table[{"factorp", "K_MOD"}] = {"ε"};
    ll1table[{"factorp", "K_OR"}] = {"ε"};
    ll1table[{"factorp", "K_AND"}] = {"ε"};
    ll1table[{"factorp", "K_LS_THEN"}] = {"ε"};
    ll1table[{"factorp", "K_GT_THEN"}] = {"ε"};
    ll1table[{"factorp", "K_EQL_TO"}] = {"ε"};
    ll1table[{"factorp", "K_LS_EQL"}] = {"ε"};
    ll1table[{"factorp", "K_GR_EQL"}] = {"ε"};
    ll1table[{"factorp", "K_NOT_EQL"}] = {"ε"};
    ll1table[{"factorp", "K_RBRACKET"}] = {"ε"};
    
    ll1table[{"factorp", "K_SEMI_COL"}] = {"ε"}; // grammer modification
    ll1table[{"factorp", "K_FED"}] = {"ε"}; // grammer modification
    ll1table[{"factorp", "K_ELSE"}] = {"ε"}; // grammer modification
    ll1table[{"factorp", "K_FI"}] = {"ε"}; // grammer modification


    // Expression Sequence:
    ll1table[{"exprseq", "K_LPAREN"}] = {"expr", "exprseqp"};
    ll1table[{"exprseq", "K_RPAREN"}] = {"ε"};
    ll1table[{"exprseq", "T_IDENTIFIER"}] = {"expr", "exprseqp"};
    // Mising from grammer --> needed modification
    ll1table[{"exprseq", "T_DOUBLE"}] = {"T_DOUBLE", "exprseqp"}; 
    ll1table[{"exprseq", "T_INT"}] = {"T_INT", "exprseqp"}; 

    // Expression Sequence Prime:
    ll1table[{"exprseqp", "K_RPAREN"}] = {"ε"};
    ll1table[{"exprseqp", "K_COMMA"}] = {"K_COMMA", "exprseq"};

    // Boolean Expression:
    ll1table[{"bexpr", "K_LPAREN"}] = {"bterm", "bexprp"};
    ll1table[{"bexpr", "K_NOT"}] = {"bterm", "bexprp"};
    ll1table[{"bexpr", "T_IDENTIFIER"}] = {"bterm", "bexprp"};
    ll1table[{"bexpr", "T_INT"}] = {"T_INT", "comp", "expr"}; // grammer modification


    // Boolean Expression Prime:
    ll1table[{"bexprp", "K_RPAREN"}] = {"ε"};
    ll1table[{"bexprp", "K_THEN"}] = {"ε"};
    ll1table[{"bexprp", "K_DO"}] = {"ε"};
    ll1table[{"bexprp", "K_OR"}] = {"K_OR", "bterm", "bexprp"};

    // Boolean Term:
    ll1table[{"bterm", "K_LPAREN"}] = {"bfactor", "btermp"};
    ll1table[{"bterm", "K_NOT"}] = {"bfactor", "btermp"};
    ll1table[{"bterm", "T_IDENTIFIER"}] = {"bfactor", "btermp"};

    // Boolean Term Prime:
    ll1table[{"btermp", "K_RPAREN"}] = {"ε"};
    ll1table[{"btermp", "K_THEN"}] = {"ε"};
    ll1table[{"btermp", "K_DO"}] = {"ε"};
    ll1table[{"btermp", "K_OR"}] = {"ε"};
    ll1table[{"btermp", "K_AND"}] = {"K_AND", "bfactor", "btermp"};

    // Boolean Factor:
    ll1table[{"bfactor", "K_LPAREN"}] = {"K_LPAREN", "bexpr", "K_RPAREN"};
    // ll1table[{"bfactor", "K_LPAREN"}] = {"expr", "comp", "expr"};
    ll1table[{"bfactor", "K_NOT"}] = {"K_NOT", "bfactor"};
    ll1table[{"bfactor", "T_IDENTIFIER"}] = {"expr", "comp", "expr"};

    // Comparison:
    ll1table[{"comp", "K_LS_THEN"}] = {"K_LS_THEN"};
    ll1table[{"comp", "K_GT_THEN"}] = {"K_GT_THEN"};
    ll1table[{"comp", "K_EQL_TO"}] = {"K_EQL_TO"};
    ll1table[{"comp", "K_LS_EQL"}] = {"K_LS_EQL"};
    ll1table[{"comp", "K_GR_EQL"}] = {"K_GR_EQL"};
    ll1table[{"comp", "K_NOT_EQL"}] = {"K_NOT_EQL"};

    // Variable:
    ll1table[{"var", "T_IDENTIFIER"}] = {"id", "varp"};

    // Variable Prime:
    ll1table[{"varp", "K_SEMI_COL"}] = {"ε"};
    ll1table[{"varp", "K_RPAREN"}] = {"ε"};
    ll1table[{"varp", "K_COMMA"}] = {"ε"};
    ll1table[{"varp", "K_EQL"}] = {"ε"};
    ll1table[{"varp", "K_LBRACKET"}] = {"K_LBRACKET", "expr", "K_RBRACKET"};

    // Identifier:
    ll1table[{"id", "T_IDENTIFIER"}] = {"T_IDENTIFIER"};
}

/**
 * AST Parsing Functions (DFS)
 * - Building Symbol Table
 *    - getting function params
 *    - getting variable declarations
 * - Semantic Analysis (return list of node types for analysis)
 *    - regular expressions
 *    - boolean expressions
 *    - arguments from function calls
*/
// Building symbol table:
void extractParams(const shared_ptr<ASTNode>& paramsNode, vector<pair<string, string>>& parameters) {
    if (!paramsNode) return;

    string paramType;
    string paramName;

    for (const auto& child : paramsNode->children) {
        // Get var type
        if (child->nodeType == "type" && !child->children.empty()) {
            paramType = child->children.front()->nodeType;
        // Get var name
        } else if (child->nodeType == "var" && !child->children.empty()) {
            paramName = child->children.front()->children.front()->value;
        // Recurse
        } else if (child->nodeType == "paramsp" && child->children.front()->nodeType != "ε") {
            extractParams(child->children[1], parameters);
        }
    }

    // Only add parameters that have both type and name defined
    if (!paramType.empty() && !paramName.empty()) {
        parameters.emplace_back(paramType, paramName);
    }
}

void extractVars(const shared_ptr<ASTNode>& varlistNode, const shared_ptr<SymbolTable>& table, string type) {
    if (!varlistNode) return;

    string varName;
    for (const auto& child : varlistNode->children) {
        if (child->nodeType == "var") {
            varName = child->children.front()->children.front()->value;
        } else if (child->nodeType == "varlistp" && child->children.front()->nodeType != "ε") {
            extractVars(child->children[1], table, type);
        }
    }

    // Only add vars that have varname defined
    if (!varName.empty()) {
        SymbolEntry entry;
        entry.type = type;
        entry.varName = varName;
        table->addEntry(varName, entry);
    }
}

// Performing Semantic Analysis
void extractExpr(const shared_ptr<ASTNode>& exprNode, vector<shared_ptr<ASTNode>>& varList) {
    if (!exprNode) return;

    // Add vars to list:
    if (exprNode->nodeType == "T_IDENTIFIER" || exprNode->nodeType == "T_DOUBLE" || exprNode->nodeType == "T_INT") varList.push_back(exprNode);

    // Recurse
    for (const auto& child : exprNode->children) extractExpr(child, varList);
}

void extractBexpr(const shared_ptr<ASTNode>& bexprNode, vector<shared_ptr<ASTNode>>& bexprList, string& comp) {
    if (!bexprNode) return;

    // Add vars to list
    if (bexprNode->nodeType == "T_IDENTIFIER" || bexprNode->nodeType == "T_INT") bexprList.push_back(bexprNode);

    // Get comp type:
    if (bexprNode->nodeType == "comp") comp = bexprNode->children.front()->nodeType;

    // Recurse
    for (const auto& child: bexprNode->children) extractBexpr(child, bexprList, comp);
}

void extractArgs(const shared_ptr<ASTNode>& argNode, vector<vector<shared_ptr<ASTNode>>>& argList, vector<shared_ptr<ASTNode>>& arg) {
    if (!argNode) 
        return;

    // Populate current arg with nodes:
    if (argNode->nodeType == "T_IDENTIFIER" || argNode->nodeType == "T_INT" || argNode->nodeType == "T_DOUBLE") 
        arg.push_back(argNode);

    // args delimited by comma --> pushback current arg to arglist and reset arg
    else if (argNode->nodeType == "K_COMMA") {
        argList.push_back(arg); 
        arg.clear();
    }

    // Push back current arg if empty expression
    else if (argNode->nodeType == "exprseqp" && argNode->children.front()->nodeType == "ε") {
        argList.push_back(arg);
        arg.clear();
    }

    // Recurse
    for (const auto& child: argNode->children) extractArgs(child, argList, arg);
}

// Generates symbol table from parse tree:
void populateSymbolTable(shared_ptr<ASTNode> &node, shared_ptr<SymbolTable>& table) {
    if (!node) return;

    // If function, if or while --> create child symbol table and set that to scope
    if (node->nodeType == "K_DEF" || node->nodeType == "K_IF" || node->nodeType == "K_WHILE") {
        SymbolEntry entry;
        entry.type = node->nodeType;
        entry.childTable = make_shared<SymbolTable>(node->nodeType, table);

        // If function get fname, type and function params
        if (node->nodeType == "K_DEF") {
            auto parent = node->parent.lock(); // K_DEF --> fdec
            // Loop through children of fdec and grab vals
            for (const auto& child : parent->children) {
                if (child->nodeType == "type") entry.returnType = child->children.front()->nodeType; // fdec->type->K_INT or K_DOUBLE
                if (child->nodeType == "fname") entry.varName = child->children.front()->children.front()->value; // fdec->fname->id->T_IDENTIFIER
                if (child->nodeType == "params") {
                    extractParams(child, entry.params); // recursively extract params
                    reverse(entry.params.begin(), entry.params.end());
                }
            }
            table->addEntry(entry.varName, entry);
        }
        table = entry.childTable;
    }

    // If variable declaration
    else if (node->nodeType == "K_INT" || node->nodeType == "K_DOUBLE") {
        auto varlistNode = node->parent.lock()->parent.lock()->children[1]; // K_INT --> type --> decl --> varlist
        if (varlistNode->nodeType == "varlist") extractVars(varlistNode, table, node->nodeType);
    }

    // Exit Scope:
    else if (node->nodeType == "K_FED" || node->nodeType == "K_FI" || node->nodeType == "K_OD") {
        table = table->parentTable;
    }

    // Process all nodes:
    for (auto& child: node->children) {
        populateSymbolTable(child, table);
    }
}

/* Phases */
void lexicalAnalysis(vector<Token>& tokenList) {
	// Initialize: temp token for storing, line and character for tracking position
    Token token;
    bool isFirstToken = true; 

    while (true) {
        token = getNextToken(); 
        if (token.type == TokenType::T_EOF) {
            break;
        }
        if (!token.isBlank()) {
            tokenList.push_back(token); // add token to list
			// Convert token.buffer (vector<char>) to string for printing
            string tokenContent(token.buffer.begin(), token.buffer.end());
            string tokenTypeStr = tokenTypeToString(token.type);

            if (!isFirstToken) {
                tokenFile << endl; 
            }
            tokenFile << "<" << tokenContent << ", " << tokenTypeStr << ">";
            isFirstToken = false; 
        }
    }
}

// Parses Token File and returns abstract syntax tree
shared_ptr<ASTNode> syntaxAnalysis() {
    auto root = make_shared<ASTNode>("S'"); // Start of tree
    // Start syntax analysis if parsing if first production is correct:
    parseTokens();
    if (ll1table[{"S'", tokenType}].empty()) errorFile << "Syntax Error: No matching production found" << endl;
    else recursiveDecent("S'", root, root); 

    printAST(root);
    cout << "Parsing Done" << endl;
    return root;
}

// Builds symbol table from AST
shared_ptr<SymbolTable> generateSymbolTable(shared_ptr<ASTNode> root) {
    auto rootSymbolTable = make_shared<SymbolTable>("global");
    populateSymbolTable(root, rootSymbolTable);

    cout << "Done Building symbol Table" << endl;
    return rootSymbolTable;
}

// Perform semantic checking
void semanticAnalysis(shared_ptr<ASTNode> node, shared_ptr<SymbolTable> table) {
    if (!node) return;

    // Update Scope for tracking
    if (node->nodeType == "fdec") 
        scope = node->children[2]->children.front()->children.front()->value;
    else if (node->nodeType == "K_FED") 
        scope = "global";

    /** 
     * Statement containing boolean expression
     * Check following semantics
     * - for var that is T_IDENTIFIER --> check scope
     * - both operands should be K/T_INT
    */
    if (node->nodeType == "statement" && node->children[1]->nodeType == "bexpr") {
        vector<shared_ptr<ASTNode>> bexprList;

        // If Scope is function --> get symbol entry and function table
        if (scope != "global") {
            auto functionEntry = table->findEntry(scope);
            auto functionTable = functionEntry->childTable;
            string comp;

            extractBexpr(node->children[1], bexprList, comp); // Extract operands from boolean expression

            // Perform Semantic checking
            for (const auto& var : bexprList) {
                if (functionTable->findEntry(var->value)) {
                    auto varEntry = functionTable->findEntry(var->value);
                    if (varEntry->type != "K_INT") errorFile << "Type Error at " << var->value << " in " << scope << endl;
                }
                else if (var->nodeType == "T_INT") continue;
                else {
                    bool found = false;
                    for (const auto& p : functionEntry->params) {
                        if (p.second == var->value) {
                            found = true; 
                            if (p.first != "K_INT") errorFile << "Type Error at " << var->value << " in " << scope << endl;
                            break;
                        }
                    }
                    if (!found) errorFile << "Declaration Error at " << var->value << " in " << scope << endl;
                }
            }
        }

        // Global Scope --> get symbol entries
        else {
            string comp;
            extractBexpr(node->children[1], bexprList, comp); // Extract operands from boolean expression
            
            // Perform Semantic checking
            for (const auto& var : bexprList) {
                if (table->findEntry(var->value)) {
                    auto varEntry = table->findEntry(var->value);
                    if (varEntry->type != "K_INT") errorFile << "Type Error at " << var->value << " in " << scope << endl;
                }
                else if (var->nodeType == "T_INT") continue;
                else errorFile << "Declaration Error at " << var->value << " in " << scope << endl;
            }
        }
    }
    
    /** 
     * Statement containing var and expression
     * Check following semantics
     * - all vars should be in scope
     * - all operands should be matching a = b * c --> typeof(a=b=c)
    */   
    else if (node->nodeType == "statement" && node->children.front()->nodeType == "var") {
        vector<shared_ptr<ASTNode>> varList;
        varList.push_back(node->children.front()->children.front()->children.front()); // add first var
        string stmtType; // stores type of first var in expression 
    
        // Perform semantic check on function and global scope expressions 
        if (scope != "global") {
            auto functionEntry = table->findEntry(scope);
            auto functionTable = functionEntry->childTable;

            // Check for first var in function scope or function params --> else declaration error
            if (functionTable->findEntry(varList.front()->value)) {
                auto varEntry = functionTable->findEntry(varList.front()->value);
                stmtType = varEntry->type;
            }
            
            else {
                bool found = false;
                for (const auto& p : functionEntry->params) {
                    if (p.second == varList.front()->value) {
                        found = true; 
                        stmtType = p.first;
                        break;
                    }
                }
                if (!found) errorFile << "Declaration Error at " << varList.front()->value << " in " << scope << endl;
            }
        
            // Extract expression vars and perform semantic checks (scope then type)
            extractExpr(node->children[2], varList);
            for (const auto& var : varList) {
                // Check for var in function scope, or if var is a int/double literal or function params --> else declaration error
                if (functionTable->findEntry(var->value)) {
                    auto varEntry = functionTable->findEntry(var->value);
                    
                    // If expression var is a function declaration --> extract args and perform sematic check
                    if (varEntry->type == "K_DEF") {
                        if (varEntry->returnType != stmtType) errorFile << "Type Error: Function " << var->value << " does not return " << stmtType << " in " << scope << endl;
                        vector<vector<shared_ptr<ASTNode>>> argList; // entire function argument
                        vector<shared_ptr<ASTNode>> arg; // indivdual args
                        auto argNode = var->parent.lock()->parent.lock()->children[1]->children[1];
                        extractArgs(argNode, argList, arg);

                        /**
                         * Semantic Check on function args
                         * - Check for num of params
                         * - compare the return type of each param and argNode in argList
                        */
                        if (varEntry->params.size() != argList.size()) errorFile << "Error: Mismatch in function call params " << varEntry->varName << " in " << scope << endl;
                        else {
                            for (size_t i = 0; i < varEntry->params.size(); i++) {
                                const auto& pType = varEntry->params[i].first; // current function param
                                const auto& arg = argList[i]; // list of vars in current arg

                                // Perform semantic on each var in arg
                                for (const auto& a: arg) {
                                    // lookup arg in symbol table and then compare p with a type
                                    // or if its a T_INT, double handle accordingly

                                    // If Arg is a identifier --> look for it in either function table or params and perform semantic check on type 
                                    if (a->nodeType == "T_IDENTIFIER") {
                                        if (functionTable->findEntry(a->value)) {
                                            auto argEntry = functionTable->findEntry(a->value);
                                            if (argEntry->type != pType) errorFile << "Error: Type Mismatch in Function Call at " << a->value << " in " << scope << endl;
                                        }
                                        else {
                                            bool found = false;
                                            for (const auto& p : functionEntry->params) {
                                                if (p.second == a->value) {found = true; break;}
                                            }
                                            if (!found) errorFile << "Declaration Error at " << a->value << " in function call in " << scope << endl;
                                        }
                                    }
                                    else if (a->nodeType == "T_INT" || a->nodeType == "T_DOUBLE") {
                                        if (a->nodeType == "T_INT" && pType == "K_INT") continue;
                                        else if (a->nodeType == "T_DOUBLE" && pType == "K_DOUBLE") continue;
                                        else errorFile << "Type Error at " << a->value << " in function call in " << scope << endl;
                                    }
                                }
                            }
                        }
                        break;
                    }
                    else if (varEntry->type != stmtType) errorFile << "Type Error at " << var->value << " in " << scope << endl;
                } 
                else if (var->nodeType == "T_INT" || var->nodeType == "T_DOUBLE") {
                    if (var->nodeType == "T_INT" && stmtType == "K_INT") continue;
                    else if (var->nodeType == "T_DOUBLE" && stmtType == "K_DOUBLE") continue;
                    else errorFile << "Type Error at " << var->value << " in " << scope << endl;
                }
                // Check for var in function params
                else {
                    bool found = false;
                    for (const auto& p : functionEntry->params) {
                        if (p.second == var->value) {
                            found = true; 
                            if (p.first != stmtType) errorFile << "Type Error at " << var->value << " in " << scope << endl;
                            break;
                        }
                    }
                    if (!found) errorFile << "Declaration Error at " << var->value << " in " << scope << endl;
                }
            }
        }

        else {
            // Check for first var global scope and assign statement type
            if (table->findEntry(varList.front()->value)) {
                auto varEntry = table->findEntry(varList.front()->value);
                stmtType = varEntry->type;
            } else errorFile << "Declaration Error at " << varList.front()->value << " in " << scope << endl;

            // Extract expression vars and perform semantic checks (scope then type)
            extractExpr(node->children[2], varList);
            for (const auto& var : varList) {
                if (table->findEntry(var->value)) {
                    auto varEntry = table->findEntry(var->value);
                    // If expression var is a function declaration --> extract args and perform sematic check
                    if (varEntry->type == "K_DEF") {
                        if (varEntry->returnType != stmtType) errorFile << "Type Error: Function " << var->value << " does not return " << stmtType << " in " << scope << endl;
                        vector<vector<shared_ptr<ASTNode>>> argList; // entire function argument
                        vector<shared_ptr<ASTNode>> arg; // indivdual args
                        auto argNode = var->parent.lock()->parent.lock()->children[1]->children[1];
                        extractArgs(argNode, argList, arg);

                        /**
                         * Semantic Check on function args
                         * - Check for num of params
                         * - compare the return type of each param and argNode in argList
                        */
                        if (varEntry->params.size() != argList.size()) errorFile << "Error: Mismatch in function call params " << varEntry->varName << " in " << scope << endl;
                        else {
                            for (size_t i = 0; i < varEntry->params.size(); i++) {
                                const auto& pType = varEntry->params[i].first; // current function param
                                const auto& arg = argList[i]; // list of vars in current arg

                                // Perform semantic on each var in arg
                                for (const auto& a: arg) {
                                    // lookup arg in symbol table and then compare p with a type
                                    // or if its a T_INT, double handle accordingly

                                    // If Arg is a identifier --> look for it in either function table or params and perform semantic check on type 
                                    if (a->nodeType == "T_IDENTIFIER") {
                                        if (table->findEntry(a->value)) {
                                            auto argEntry = table->findEntry(a->value);
                                            if (argEntry->type != pType) errorFile << "Error: Type Mismatch in Function Call at " << a->value << " in " << scope << endl;
                                        }
                                        else errorFile << "Declaration Error at " << a->value << " in function call in " << scope << endl;
                                        
                                    }
                                    else if (a->nodeType == "T_INT" || a->nodeType == "T_DOUBLE") {
                                        if (a->nodeType == "T_INT" && pType == "K_INT") continue;
                                        else if (a->nodeType == "T_DOUBLE" && pType == "K_DOUBLE") continue;
                                        else errorFile << "Type Error at " << a->value << " in function call in " << scope << endl;
                                    }
                                }
                            }
                        }
                        break;
                    }
                    else if (varEntry->type != stmtType) errorFile << "Type Error at " << var->value << " in " << scope << endl; 
                    
                    
                } 
                else if (var->nodeType == "T_INT" || var->nodeType == "T_DOUBLE") {
                    if (var->nodeType == "T_INT" && stmtType == "K_INT") continue;
                    else if (var->nodeType == "T_DOUBLE" && stmtType == "K_DOUBLE") continue;
                    else errorFile << "Type Error at " << var->value << " in " << scope << endl;
                }
                else errorFile << "Declaration Error at " << var->value << " in " << scope << endl;
            }
        }
    }

    else if (node->nodeType == "statement" && node->children.front()->nodeType == "K_RETURN" && scope != "global") {
        vector<shared_ptr<ASTNode>> varList;
        auto functionEntry = table->findEntry(scope);
        auto functionTable = functionEntry->childTable;
        string stmtType = functionEntry->returnType;
        // Extract Expression vars and perform semantic checks
        extractExpr(node->children[1], varList);
        for (const auto& var : varList) {
            // Check for var in function scope, or if var is a int/double literal or function params --> else declaration error
            if (functionTable->findEntry(var->value)) {
                auto varEntry = functionTable->findEntry(var->value);
                
                // If expression var is a function declaration --> extract args and perform sematic check
                if (varEntry->type == "K_DEF") {
                    if (varEntry->returnType != stmtType) errorFile << "Type Error: Function " << var->value << " does not return " << stmtType << " in " << scope << endl;
                    vector<vector<shared_ptr<ASTNode>>> argList; // entire function argument
                    vector<shared_ptr<ASTNode>> arg; // indivdual args
                    auto argNode = var->parent.lock()->parent.lock()->children[1]->children[1];
                    extractArgs(argNode, argList, arg);

                    /**
                     * Semantic Check on function args
                     * - Check for num of params
                     * - compare the return type of each param and argNode in argList
                    */
                    if (varEntry->params.size() != argList.size()) errorFile << "Error: Mismatch in function call params " << varEntry->varName << " in " << scope << endl;
                    else {
                        for (size_t i = 0; i < varEntry->params.size(); i++) {
                            const auto& pType = varEntry->params[i].first; // current function param
                            const auto& arg = argList[i]; // list of vars in current arg

                            // Perform semantic on each var in arg
                            for (const auto& a: arg) {
                                // lookup arg in symbol table and then compare p with a type
                                // or if its a T_INT, double handle accordingly

                                // If Arg is a identifier --> look for it in either function table or params and perform semantic check on type 
                                if (a->nodeType == "T_IDENTIFIER") {
                                    if (functionTable->findEntry(a->value)) {
                                        auto argEntry = functionTable->findEntry(a->value);
                                        if (argEntry->returnType != pType) errorFile << "Error: Type Mismatch in Function Call at " << a->value << " in " << scope << endl;
                                    }
                                    else {
                                        bool found = false;
                                        for (const auto& p : functionEntry->params) {
                                            if (p.second == a->value) {found = true; break;}
                                        }
                                        if (!found) errorFile << "Declaration Error at " << a->value << " in function call in " << scope << endl;
                                    }
                                }
                                else if (a->nodeType == "T_INT" || a->nodeType == "T_DOUBLE") {
                                    if (a->nodeType == "T_INT" && pType == "K_INT") continue;
                                    else if (a->nodeType == "T_DOUBLE" && pType == "K_DOUBLE") continue;
                                    else errorFile << "Type Error at " << a->value << " in function call in " << scope << endl;
                                }
                            }
                        }
                    }
                    break;
                }
                else if (varEntry->type != stmtType) errorFile << "Type Error at " << var->value << " in " << scope << endl;
            } 
            else if (var->nodeType == "T_INT" || var->nodeType == "T_DOUBLE") {
                if (var->nodeType == "T_INT" && stmtType == "K_INT") continue;
                else if (var->nodeType == "T_DOUBLE" && stmtType == "K_DOUBLE") continue;
                else errorFile << "Type Error at " << var->value << " in " << scope << endl;
            }
            // Check for var in function params
            else {
                bool found = false;
                for (const auto& p : functionEntry->params) {
                    if (p.second == var->value) {
                        found = true; 
                        if (p.first != stmtType) errorFile << "Type Error at " << var->value << " in " << scope << endl;
                        break;
                    }
                }
                if (!found) errorFile << "Declaration Error at " << var->value << " in " << scope << endl;
            }
        }
    }

    // Process all nodes
    for (auto& child: node->children) {
        semanticAnalysis(child, table);
    }
}

// Functions for Recursively returning 3TAC information:
void ICG_K_IF(shared_ptr<ASTNode> node, shared_ptr<SymbolTable> table, vector<string>& header3TAC, vector<string> body3TAC, int& labNum) {
    if (!node) return;

    // Append header and body of each K_IF to recursed 3TACS
    if (node->nodeType == "statement" && node->children.front()->nodeType == "K_IF") {
        string comp;
        vector<shared_ptr<ASTNode>> bexprList; 
        extractBexpr(node->children[1], bexprList, comp);

        // Get Branch Equality
        string branchEquality;
        if (comp == "K_LS_EQL") branchEquality = "BLE";
        else if (comp == "K_NOT_EQL") branchEquality = "BNE";
        else if (comp == "K_LS_THEN") branchEquality = "BLE";
        else if (comp == "K_EQL_TO") branchEquality = "BEQ";
        else if (comp == "K_GR_EQL") branchEquality = "BGE";
        else if (comp == "K_GT_THEN") branchEquality = "BGT";

        // Append to Header:
        string icg = branchEquality + " lab" + to_string(labNum);
        header3TAC.push_back(icg);
        labNum += 1;

        // Build Label Body:
        // If Body
        if (node->children[3]->children.front()->nodeType == "statement") {
            if (node->children[3]->children.front()->children.front()->nodeType == "K_RETURN") {

            }
        }
        // Else Body
        if (node->children[4]->children.front()->nodeType == "K_ELSE") {

        }
        // ICG_LAB(node->children[3], ) IF
        // ICG_LAB(node->children[4]) ELSE
    }

    // Recurse:
    for (auto& child: node->children) {
        ICG_K_IF(child, table, header3TAC, body3TAC, labNum);
    }
}

// Generate intermediate code for compiling
void createICG(shared_ptr<ASTNode> node, shared_ptr<SymbolTable> table) {

    if (!node) return;

    // Generate Function ICG(s) if they exist
    if (node->nodeType == "fdec") {

        // Get Function Information
        // node->children[2]->children.front()->children.front()->value
        auto functionEntry = table->findEntry(node->children[2]->children.front()->children.front()->value);
        auto functionTable = functionEntry->childTable;

        // Incrementation Vals
        int bytesRequired = 0; // total bytes required by function
        int labelNum = 1; // lab1, lab2, etc...
        int loopNum = 1; // loop1, loop2, etcc...

        // Printing Declarations
        vector<string> printFuncDecls; // gcd: 24 Begin, push lr and push fp, exitgcd
        vector<string> printFuncParams; // b = fp + 8, a = fp + 12
        vector<string> funcHeader3TAC; // header for 3TAC --> BEQ lab1, BLT loop1
        vector<string> funcBodyTAC; // body of 3TAC command --> lab and loops
        
        // Get function params:
        int fpCounter = 8;
        for (const auto& p : functionEntry->params) {
            string param;
            string tempCounter = to_string(fpCounter);
            
            // param = p.second + " = fp + " + tempCounter + "\n";
            printFuncParams.push_back(param);
        }

        // Handle 3TAC --> K_IF:
        vector<string> K_IF_Header3TAC;
        vector<string> K_IF_Body3TAC;

        ICG_K_IF(node, table, K_IF_Header3TAC, K_IF_Body3TAC, labelNum);

        // Handle 3TAC --> K_WHILE:
        // vector<string> K_WHILE_Header3TAC;
        // vector<string> K_WHILE_Body3TAC;

        // ICG_K_WHILE(node, table, K_IF_Header3TAC, K_IF_Body3TAC, labelNum);

        // Append to function header and body
        funcHeader3TAC.insert(funcHeader3TAC.end(), K_IF_Header3TAC.begin(), K_IF_Header3TAC.end());
        funcBodyTAC.insert(funcBodyTAC.end(), K_IF_Body3TAC.begin(), K_IF_Body3TAC.end());


    }

    // // Process all nodes:
    for (auto& child: node->children) {
        createICG(child, table);
    }
}

int main() {

    // Open Files
    string inputFilePath;
    cout << "Enter Path of file to compile: ";
    cin >> inputFilePath;

    inputFilePath = "test cases/" + inputFilePath + ".cp";

    inputFile.open(inputFilePath);
    tokenFile.open("tokens.txt");
    errorFile.open("errors.txt");

    if (!inputFile.is_open() || !tokenFile.is_open() || !errorFile.is_open()) {
        cerr << "Error opening files" << endl;
        return 1; // Return a non-zero value to indicate error
    }

    // Generate keywords, transition table and LL1 sparse map:
    generateTable(); 
    loadKeywords();
    loadLL1(); // Load ll1

    // Phase 1: Run lexical parsing and open file if succesful
    vector<Token> tokenList;
    lexicalAnalysis(tokenList); // Phase 1

    lexemmeFile.open("tokens.txt");
    if (!lexemmeFile.is_open()) {
        cerr << "Error opening files" << endl;
        return 1;
    }

    // Phase 2: Run syntax analysis to build AST and then generate symbol table
    auto root = syntaxAnalysis(); 
    auto symbolTable = generateSymbolTable(root);

    // Phase 3: Perform semantic analysis
    semanticAnalysis(root, symbolTable);

    // Phase 4: Intermediate Code Gen (only do this if code is semantically correct)
    bool isEmpty = errorFile.tellp() == 0;
    if (isEmpty) {
        ICGFile.open("compile.txt");
        ICGFile << "B main" << endl;
        createICG(root, symbolTable);
    } 
    else cout << "Source File is invalid" << endl;

    return 0;
}