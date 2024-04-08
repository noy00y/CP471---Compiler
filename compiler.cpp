/* Imports */ 
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cctype> 
#include <array>
#include <map>
#include <algorithm>
using namespace std;

/* Constants and Global Declarations: */ 
#define MAX_KEYWORDS 100
#define BUFFER_SIZE 2048

// Lexical Analysis:
ifstream inputFile;
ofstream tokenFile;
ofstream errorFile;

vector<string> keywords(33); // eg. for, do, while, etc...
array<array<int, 127>, 30> table{}; // transition table for automaton machine (30 states, 127 inputs)
vector<char> buffer1(BUFFER_SIZE); // Dual buffers for reading 
vector<char> buffer2(BUFFER_SIZE); 
vector<char>* currentBuffer = &buffer1;

/* Structs */
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

/* Classes */
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
            if (!std::isspace(static_cast<unsigned char>(ch)))
                return false;
        }
        return true;
    }

    void determineType(const std::vector<std::string>& keywords) {
        if (!isBlank()) {
            std::string tokenStr(buffer.begin(), buffer.end());

            auto it = std::find_if(keywords.begin(), keywords.end(),
                                   [&tokenStr](const std::string& keyword) {
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

// Parse file stream for chars
Token getNextToken() {
    Token token; // Token now uses its default constructor for initialization
    int currentState = 0;
    char currentChar;

    while (inputFile.get(currentChar)) { 
        int ascii = static_cast<int>(currentChar); // Use static_cast for conversions in C++
        cout << "Token: " << currentChar << " --> ascii: " << ascii << ", currentState = " << currentState << endl;
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

            return token;
        }

        else if ((currentState == 13 || currentState == 15 || currentState == 18) && (ascii <  48 || ascii > 57) && (ascii != 46) && (ascii != 69)&& (ascii != 101)) {
            // cout << "Return " << currentChar << " w/ ascii = " << ascii << " back to the file stream" << endl;
            inputFile.unget();
            if (currentState == 13) token.type = TokenType::T_INT;
            else if (currentState == 15 || currentState == 18) token.type = TokenType::T_DOUBLE;
            return token;
        }

        else if ((currentState == 10) && (ascii < 97 || ascii > 122)) {
            // cout << "Return " << currentChar << " w/ ascii = " << ascii << " back to the file stream" << endl;
            inputFile.unget(); // Put character back into stream
            token.determineType(keywords);            
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

            return token;
        }

        // Accept Single Operator --> 4, 8 --> Need to fix this because its not accepting the char that comes after
        else if (currentState == 4 || currentState == 8) {
            
            // Determine Token Type:
            if (currentState == 4) token.type = TokenType::K_GT_THEN;
            else if (currentState == 8) token.type = TokenType::K_LS_THEN;

            return token;
        }

        // Accept Identifer if at 100
        else if (currentState == 100) {
            token.determineType(keywords);        
            return token;
        }
    }
    token.type = TokenType::T_EOF;
    return token;
}

/* Functions */
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

/* Phases */
void lexicalAnalysis() {
	// Initialize: temp token for storing, line and character for tracking position
    Token token;
    bool isFirstToken = true; 

    while (true) {
        token = getNextToken(); 
        if (token.type == TokenType::T_EOF) {
            break;
        }
        if (!token.isBlank()) {
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

void syntaxAnalysis() {

}

int main() {

    // Open Files
    // string inputFilePath;
    // cout << "Enter Path of file to compile: ";
    // cin >> inputFilePath;

    inputFile.open("test cases/Test3.cp");
    tokenFile.open("tokens.txt");
    errorFile.open("errors.txt");

    if (!inputFile.is_open() || !tokenFile.is_open() || !errorFile.is_open()) {
        cerr << "Error opening files" << endl;
        return 1; // Return a non-zero value to indicate error
    }

    // Generate keywords and transition table:
    generateTable(); 
    loadKeywords();

    // Run parsing
    lexicalAnalysis();

    return 0;
}