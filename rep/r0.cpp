#include <iostream>
#include <queue>
#include <typeinfo>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "c0.h"
#include "r0.h"

using namespace std;
using namespace r0;

//#define DEBUG

P::P(const P &obj)
{
    this->e = obj.e->clone();
}

void P::deep_delete()
{
    this->e->deep_delete();
    delete this->e;
}

Num* Num::clone() const
{
    return new Num(this->value);
}

Bool* Bool::clone() const
{
    return new Bool(this->value);
}

Read* Read::clone() const
{
    return new Read();
}

Binop* Binop::clone() const
{
    return new Binop(this->op, this->l->clone(), this->r->clone());
}

Unop* Unop::clone() const
{
    return new Unop(this->op, this->v->clone());
}

Var* Var::clone() const
{
    return new Var(this->name);
}

Let* Let::clone() const
{
    return new Let(this->name, this->ve->clone(), this->be->clone());
}

// TODO fixme
static unordered_map<string, unsigned int> count;

string gensym(string sym)
{
    unsigned int id = 0;
    auto it = count.find(sym);
    if (it != count.end())
    {
        id = ++(it->second);
    }
    else
    {
        count[sym] = 0;
    }
    return sym + "_" + to_string(id);
}

void P::uniquify()
{
    count.clear();
    unordered_map<string, string> varmap;
    this->e->uniquify(varmap);
}

bool P::is_unique() const
{
    vector<string> varnames; // FIXME is vector overkill for finding duplicates?
    queue<E*> nodes;
    nodes.push(e);
    while (!nodes.empty())
    {
        E* node = nodes.front();
        nodes.pop();
        if (typeid(*node) == typeid(Let)) // track all declared vars
        {
            varnames.push_back((static_cast<Let*>(node)->name));
        }
        auto newnodes = node->get_childs();
        for (auto n : newnodes)
        {
            nodes.push(n);
        }
    }
    set<string> uniquenames(varnames.begin(), varnames.end());
    return varnames.size() == uniquenames.size();
}


void Num::uniquify(unordered_map<string, string> m)
{
    return;
}

void Bool::uniquify(unordered_map<string, string> m)
{
    return;
}

void Read::uniquify(unordered_map<string, string> m)
{
    return;
}

void Binop::uniquify(unordered_map<string, string> m)
{
    this->l->uniquify(m);
    this->r->uniquify(m);
}

void Unop::uniquify(unordered_map<string, string> m)
{
    this->v->uniquify(m);
}

void Var::uniquify(unordered_map<string, string> m)
{
    auto it = m.find(this->name); // FIXME const iterator will be the "right"
                                  // thing to do instead of auto
    if (it != m.end())
    {
#ifdef DEBUG
        cout << "Uniquify var: changing " << this->name << " to " << it->second << endl;
#endif
        this->name = it->second;
    }
}

void Let::uniquify(unordered_map<string, string> m)
{
    // uniquify the var expression first because the var expression should not
    // have access to the var it's trying to define
    this->ve->uniquify(m);

    m[this->name] = gensym(this->name);
#ifdef DEBUG
    cout << "Uniquify let: changing " << this->name << " to " << m[this->name] << endl;
#endif
    this->name = m[this->name];
    this->be->uniquify(m);
}

c0::P P::flatten() const
{
    vector<string> vars;
    vector<c0::S> stmts;
    c0::Arg* a = this->e->to_c0(vars, stmts);
    type t = this->type_check();
    if (t == ERROR)
    {
        cerr << "Type check failed";
        exit(1);
    }
    return c0::P(vars, stmts, a, t);
}


c0::Arg* Num::to_c0(vector<string> &vars, vector<c0::S> &stmts) const
{
     return new c0::Num(this->value);
}

c0::Arg* Bool::to_c0(vector<string> &vars, vector<c0::S> &stmts) const
{
     return new c0::Num(static_cast<int>(this->value));
}

c0::Arg* Read::to_c0(vector<string> &vars, vector<c0::S> &stmts) const
{
    string s = gensym("r0Read");
    vars.push_back(s);
    stmts.push_back(c0::S(s, new c0::Read()));
    return new c0::Var(s);
}

c0::Arg* Binop::to_c0(vector<string> &vars, vector<c0::S> &stmts) const
{
    string s = gensym("r0Binop");
    vars.push_back(s);
    stmts.push_back(c0::S(s, new c0::Binop(this->op,
                    this->l->to_c0(vars, stmts),
                    this->r->to_c0(vars, stmts))));
    return new c0::Var(s);
}

c0::Arg* Unop::to_c0(vector<string> &vars, vector<c0::S> &stmts) const
{
    string s = gensym("r0Unop");
    vars.push_back(s);
    stmts.push_back(c0::S(s, new c0::Unop(this->op, this->v->to_c0(vars, stmts))));
    return new c0::Var(s);
}

c0::Arg* Var::to_c0(vector<string> &vars, vector<c0::S> &stmts) const
{
    return new c0::Var(this->name);
}

c0::Arg* Let::to_c0(vector<string> &vars, vector<c0::S> &stmts) const
{
    stmts.push_back(c0::S(this->name, this->ve->to_c0(vars, stmts)));
    vars.push_back(this->name);
    return this->be->to_c0(vars, stmts);
}

type P::type_check() const
{
    unordered_map<string, type> vars;
    return this->e->t_check(vars);
}

type Num::t_check(unordered_map<string, type> vmap) const
{
    return NUM;
}

type Bool::t_check(unordered_map<string, type> vmap) const
{
    return BOOL;
}

type Read::t_check(unordered_map<string, type> vmap) const
{
    return NUM;
}

type Binop::t_check(unordered_map<string, type> vmap) const
{
    type lt = this->l->t_check(vmap);
    type rt = this->r->t_check(vmap);
    if (lt == ERROR || rt == ERROR)
    {
        cerr << "Binop: args have unresolvable types\n";
        return ERROR;
    }
    if (this->op == B_PLUS) // num,num -> num
    {
        if (lt == NUM && rt == NUM)
        {
            return NUM;
        }
        else
        {
            cerr << "Expected num,num got " << lt << ", " << rt << endl;
            return ERROR;
        }
    }
    else if ((this->op == B_EQ) || // num, num -> bool
             (this->op == B_LT) ||
             (this->op == B_GT) ||
             (this->op == B_LE) ||
             (this->op == B_GE))
    {
        if (lt == NUM && rt == NUM)
        {
            return BOOL;
        }
        else
        {
            cerr << "Expected num,num got " << lt << ", " << rt << endl;
            return ERROR;
        }
    }
    else
    {
        cerr << "Unsupported operator: " << this->op << endl;
        return ERROR;
    }
}

type Unop::t_check(unordered_map<string, type> vmap) const
{
    type vt = this->v->t_check(vmap);
    if (vt == ERROR)
    {
        cerr << "Unop: child has unresolved type\n";
        return ERROR;
    }
    if (this->op == U_NEG) // num -> num
    {
        if (vt == NUM)
        {
            return NUM;
        }
        else
        {
            cerr << "U_NEG expects a NUM\n";
            return ERROR;
        }
    }
    else if (this->op == U_NOT) // bool -> bool
    {
        if (vt == BOOL)
        {
            return BOOL;
        }
        else
        {
            cerr << "U_NOT expects a BOOL\n";
            return ERROR;
        }
    }
    else
    {
        cerr << "Bad unary op " << this->op << endl;
        return ERROR;
    }
}

type Var::t_check(unordered_map<string, type> vmap) const
{
    return vmap.at(this->name);
}

type Let::t_check(unordered_map<string, type> vmap) const
{
    vmap[this->name] = this->ve->t_check(vmap);
    return this->be->t_check(vmap);
}

