#include <iostream>
#include <string>
#include <vector>
#include <memory>

using namespace std;

// Base class for AST nodes
class ASTNode {
public:
    string value;
    vector<shared_ptr<ASTNode>> children;

    ASTNode(const string& val) : value(val) {}
    virtual ~ASTNode() = default;

    void addChild(shared_ptr<ASTNode> child) {
        children.push_back(child);
    }
};

// Global temporary variable counter
int tempCount = 1;

// Function to generate a new temporary variable
string newTemp() {
    return "t" + to_string(tempCount++);
}

// Function to generate 3TAC from an AST node
string generate3TAC(shared_ptr<ASTNode> node) {
    if (node->children.empty()) {
        return node->value; // Base case: if no children, return node value (identifier or number)
    }

    string left, right, temp;
    if (node->value == "=") {
        // Assignment operation, expect the first child to be an ID and second to be an expression
        cout << "Node Val --> =: " << node->value << endl;
        string id = generate3TAC(node->children[0]);
        string expr = generate3TAC(node->children[1]);
        cout << id << " = " << expr << endl;
        return "";
    } else if (node->value == "+" || node->value == "-" || node->value == "*" || node->value == "%") {
        // Binary operators
        cout << "Node Val --> +, -, etc... " << node->value << endl;
        left = generate3TAC(node->children[0]);
        right = generate3TAC(node->children[1]);
        temp = newTemp();
        cout << temp << " = " << left << " " << node->value << " " << right << endl;
        return temp;
    }

    return "";
}

int main() {
    // Constructing the AST manually based on the given tree structure
    auto root = make_shared<ASTNode>("=");
    auto r = make_shared<ASTNode>("r");
    root->addChild(r);

    auto expr = make_shared<ASTNode>("+");
    root->addChild(expr);

    auto term1 = make_shared<ASTNode>("*");
    expr->addChild(term1);

    auto f = make_shared<ASTNode>("f");
    term1->addChild(f);
    auto n = make_shared<ASTNode>("n");
    term1->addChild(n);

    auto plus1 = make_shared<ASTNode>("+");
    expr->addChild(plus1);

    auto a = make_shared<ASTNode>("a");
    plus1->addChild(a);

    auto plus2 = make_shared<ASTNode>("+");
    plus1->addChild(plus2);

    auto term2 = make_shared<ASTNode>("*");
    plus2->addChild(term2);

    auto mod = make_shared<ASTNode>("%");
    term2->addChild(mod);

    auto o = make_shared<ASTNode>("o");
    mod->addChild(o);
    auto b = make_shared<ASTNode>("b");
    mod->addChild(b);

    auto q = make_shared<ASTNode>("q");
    term2->addChild(q);

    auto plus3 = make_shared<ASTNode>("+");
    plus2->addChild(plus3);

    auto c = make_shared<ASTNode>("c");
    plus3->addChild(c);

    auto minus = make_shared<ASTNode>("-");
    plus3->addChild(minus);

    auto z = make_shared<ASTNode>("z");
    minus->addChild(z);

    // Generate 3TAC
    cout << "Result: " << generate3TAC(root) << endl;

    return 0;
}
