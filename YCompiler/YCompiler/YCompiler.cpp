#define _CRT_SECURE_NO_WARNINGS

/****************************************************************/
//	编译原理课程设计
//	
//	https://github.com/YiChuan0712/SNL-Compiler
//
//	张一川 2021年四月
//
//	包含以下四个模块
//		词法分析模块
//		语法分析模块（递归下降方法）
//		语法分析模块（LL(1)方法）
//		语义分析模块（大部分的语法错误都能检查）
//
/****************************************************************/

#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <fstream> 
#include <string>

using namespace std;

/****************************************************************/
/****************************************************************/
// 输入输出文件路径
///*
char SOURCE[100] = "C:\\Users\\Yichuan\\Desktop\\source.txt"; // SOURCE 源代码
char OUT1[100] = "C:\\Users\\Yichuan\\Desktop\\token.txt"; // 词法分析
char OUT2a[100] = "C:\\Users\\Yichuan\\Desktop\\tree.txt"; // 递归下降
char OUT2b[100] = "C:\\Users\\Yichuan\\Desktop\\LL1.txt"; // LL1
char OUT3[100] = "C:\\Users\\Yichuan\\Desktop\\table.txt"; // 语义分析 符号表
char OUTerror[100] = "C:\\Users\\Yichuan\\Desktop\\error.txt"; // 报错
//*/
/****************************************************************/
/****************************************************************/
// 使用的各种全局变量和数据结构
// 词法分析
typedef enum
{
	ENDFILE, ERROR,
	PROGRAM, PROCEDURE, TYPE, VAR, IF,
	THEN, ELSE, FI, WHILE, DO, ENDWH,
	BEGIN, END1, READ, WRITE, ARRAY, OF,
	RECORD, RETURN1,
	INTEGER, CHAR1, //INTEGER 是integer
	ID, INTC, CHARC,
	ASSIGN, EQ, LT, PLUS, MINUS,
	TIMES, OVER, LPAREN, RPAREN, DOT,
	COLON, SEMI, COMMA, LMIDPAREN, RMIDPAREN,
	UNDERANGE
}LexType;// 终极符

// 保留字
static struct Word
{
	string str;
	LexType tok;
}
reservedWords[21] =
{
	{"program",PROGRAM},
	{"type",TYPE},
	{"var",VAR},
	{"procedure",PROCEDURE},
	{"begin",BEGIN},
	{"end",END1},
	{"array",ARRAY},
	{"of",OF},
	{"record",RECORD},
	{"if",IF},{"then",THEN},
	{"else",ELSE},
	{"fi",FI},
	{"while",WHILE},
	{"do",DO},
	{"endwh",ENDWH},
	{"read",READ},
	{"write",WRITE},
	{"return",RETURN1},
	{"integer",INTEGER},
	{"char",CHAR1}
};

struct Token {
	int line;
	struct Word wd;
	int index = -1; // tokenlist中的index
};// token

Token tokenList[1024];// token表
/****************************************************************/
// 语法分析
class TreeNode
{
public:
	string name;
	Token* tk;
	TreeNode* child[32];
	int childIndex;
	TreeNode(string nm)
	{
		name = nm;
		tk = NULL;
		for (int i = 0; i < 31; i++)
		{
			child[i] = NULL;
		}
		childIndex = 0;
	}
	void addChild(TreeNode* t) {
		child[childIndex] = t;
		childIndex++;
	}
};

typedef enum
{
	Program, ProgramHead, ProgramName, DeclarePart,
	TypeDec, TypeDeclaration, TypeDecList, TypeDecMore,
	TypeId, TypeName, BaseType, StructureType,
	ArrayType, Low, Top, RecType,
	FieldDecList, FieldDecMore, IdList, IdMore,
	VarDec, VarDeclaration, VarDecList, VarDecMore,
	VarIdList, VarIdMore, ProcDec, ProcDeclaration,
	ProcDecMore, ProcName, ParamList, ParamDecList,
	ParamMore, Param, FormList, FidMore,
	ProcDecPart, ProcBody, ProgramBody, StmList,
	StmMore, Stm, AssCall, AssignmentRest,
	ConditionalStm, StmL, LoopStm, InputStm,
	InVar, OutputStm, ReturnStm, CallStmRest,
	ActParamList, ActParamMore, RelExp, OtherRelE,
	Exp, OtherTerm, Term, OtherFactor,
	Factor, Variable, VariMore, FieldVar,
	FieldVarMore, CmpOp, AddOp, MultOp
}NonTerminal; // 非终极符

typedef LexType Terminal; //终极符的别名

// LL1分析栈的一个节点
struct StackNode
{
	int ntflag;

	NonTerminal n;//0
	Terminal t;//1

	TreeNode* p;

	struct StackNode *next;

};
// LL1分析栈
class AnalysisStack
{
public:
	AnalysisStack()
	{
		isEmpty = 1;
		top = NULL;
	}
	StackNode* top; // 注意 pop() 无返回值 查看栈顶元素请用top
	int isEmpty;
	void push(int ntflag, int num)
	{
		StackNode *p;
		p = new StackNode();
		p->next = NULL;
		p->ntflag = -1; //正常应为 0 1
		p->n = (NonTerminal)-1;//按错误的值进行初始化
		p->t = (Terminal)-1;//按错误的值进行初始化


		if (ntflag == 0)
			p->n = (NonTerminal)num;
		else if (ntflag == 1)
			p->t = (Terminal)num;

		p->ntflag = ntflag;
		p->next = top;
		top = p;
		isEmpty = 0;
	}
	void pop()
	{
		top = top->next;
		if (top == NULL)
			isEmpty = 1;
	}
};

AnalysisStack* anlsstack = new AnalysisStack();
/****************************************************************/
// 语义分析
class SymbolTable;
// 符号表的记录节点
struct SymbolRecord
{
	string name; // 名字
	string kind; // 三种类型
	string type; // 具体类型
	Token* tk; // 指向tokenlist
	SymbolTable * next;
};

// 符号表
void printErrorMsg(string);
string outputstr;
class SymbolTable
{
public:
	SymbolTable()
	{
		index = 0;
		paramcount = 0;
	}
	SymbolRecord* table[512];
	int index;
	int paramcount;
	void addRecord(string name, string kind, string type, Token* tk, SymbolTable *next = NULL)
	{
		for (int i = 0; i < index; i++)
		{
			if (name == table[i]->name)
			{
				string temp = "Line ";
				temp += to_string(tk->line);
				temp += " ";
				temp += "\"";
				temp += name;
				temp += "\" ";
				temp += "RepeatDeclaration Error!\n";
				printErrorMsg(temp);
				return;
			}
		}
		table[index] = new SymbolRecord();
		char temp[100];

		table[index]->name = name;
		table[index]->kind = kind;
		table[index]->type = type;
		table[index]->tk = tk;
		table[index]->next = next;
		index++;
	}
	void printTable(int layer = 0)
	{
		for (int i = 0; i < index; i++)
		{
			for (int i = 0; i < layer; i++)
			{
				cout << "\t\t\t\t\t";
				outputstr += "\t\t\t\t\t";
			}

			cout << table[i]->name
				<< "\t"
				<< table[i]->kind
				<< "\t"
				<< table[i]->type << endl;

			outputstr += table[i]->name;
			outputstr += "\t";
			outputstr += table[i]->kind;
			outputstr += "\t";
			outputstr += table[i]->type;
			outputstr += "\n";

			if (table[i]->next)
			{
				//cout << endl;
				//outputstr += "\n";
				table[i]->next->printTable(layer + 1);
				cout << endl;
				outputstr += "\n";
			}
		}

		ofstream output(OUT3);
		output << outputstr;
	}
};

SymbolTable* smbltable = new SymbolTable();

// 全局变量 LL1树根结点 设为全局变量是为了方便在语义分析中继续使用
TreeNode* treeroot = NULL;

/****************************************************************/
/****************************************************************/

int INDEX = 0; // 搭配tokenList使用
Token* currentToken; // 建立语法树时使用

string outstr[512]; // 打印语法树时使用 注意我递归下降和LL1的语法树是分别保存的 没有重复使用同一函数
int strline = 0; //这是为了避免在覆盖读写string时出错

string outstrLL1[512]; //
int strlineLL1 = 0;

char ch1[3] = "├"; // 全角字符用来画树枝 ch1[0] ch1[1] 保存了字符 ch1[2]是\0 
char ch2[3] = "└"; // 全角字符用起来真的很容易出错...
char ch3[3] = "│";

int LL1Table[104][104]; // LL1表 这里采用了书上的做法 用一个单独的函数进行初始化
TreeNode *currentTree; // LL1用

/****************************************************************/
/****************************************************************/
bool isSeparater(char ch);
bool isOperator(char ch);
bool isReserved(string ch);
bool isBlank(char ch);
bool isLetter(char ch);
bool isDigit(char ch);
void lexicalAnalyse(FILE *fp);
struct Word getReserved(string s);
string getString(int lexNum);
void printTokenList();

// 递归下降建立语法树时所用的函数 因为递归函数是一层套一层调用的所以需要先全部声明出来
TreeNode* program();//
TreeNode* programHead();//
TreeNode* declarePart();
TreeNode* programBody();
TreeNode* typeDecPart();
TreeNode* varDecPart();
TreeNode* procDecpart();
TreeNode* typeDec();
TreeNode* typeDecList();
// TreeNode* typeID();
TreeNode* typeDef();
TreeNode* typeDecMore();
TreeNode* baseType();
TreeNode* structureType();
TreeNode* arrayType();
TreeNode* recType();
TreeNode* fieldDecList();
TreeNode* IDList();
TreeNode* fieldDecMore();
TreeNode* IDMore();
TreeNode* varDec();
TreeNode* varDecList();
TreeNode* varIDList();
TreeNode* varDecMore();
TreeNode* varIDMore();
TreeNode* procDec();
TreeNode* paramList();
TreeNode* procDecPart();
TreeNode* procBody();
TreeNode* paramDecList();
TreeNode* param();
TreeNode* paramMore();
TreeNode* formList();
TreeNode* fidMore();
TreeNode* stmList();
TreeNode* stm();
TreeNode* stmMore();
TreeNode* conditionalStm();
TreeNode* loopStm();
TreeNode* inputStm();
TreeNode* outputStm();
TreeNode* returnStm();
TreeNode* assCall();
TreeNode* assignmentRest();
TreeNode* callStmRest();
TreeNode* variMore();
TreeNode* exp();
TreeNode* actparamList();
TreeNode* actparamMore();
TreeNode* term();
TreeNode* otherTerm();
TreeNode* addOp();
TreeNode* factor();
TreeNode* otherFactor();
TreeNode* multOp();
TreeNode* variable();
TreeNode* fieldVar();
TreeNode* fieldVarMore();

void printErrorMsg(string reason);

void printTree(TreeNode * t, int layer, bool islastson, char out[100]);

void processAddChild(int istmnl, int lex, string name, int childindex);

void process(int num);

void InitLL1Table();

TreeNode* programLL1();

/****************************************************************/
/****************************************************************/

// digit check
// 检查是不是数字
bool isDigit(char ch)
{
	if (ch >= '0' && ch <= '9')
		return true;
	else
		return false;
}

// letter check
// 检查是不是字母
bool isLetter(char ch)
{
	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
		return true;
	else
		return false;
}

// blank check
// 检查是不是需要blank掉
bool isBlank(char ch)
{

	if (ch == ' ') // 空格
		return true;
	else if (ch == '\n') // 换行
		return true;
	else if (ch == '\t') // 制表符
		return true;
	else if (ch == '\r') // 回车
		return true;
	else
		return false;
}

// key check
// 检查是不是保留字
bool isReserved(string ch)
{
	for (int i = 0; i < 21; i++)
		if (ch == reservedWords[i].str)
			return true;
	return false;
}

// find the correspond reversed word
// 在保留字表中找到对应的编号
struct Word getReserved(string s)
{
	for (int i = 0; i < 21; i++)
	{
		if (reservedWords[i].str == s)
			return reservedWords[i];
	}
}

// operator check
// 检查是不是运算符
bool isOperator(char ch)
{
	if (ch == '+')
		return true;
	else if (ch == '-')
		return true;
	else if (ch == '*')
		return true;
	else if (ch == '/')
		return true;
	else if (ch == '<')
		return true;
	else if (ch == '=')
		return true;
	else
		return false;
}

// separater cehck
// 检查是不是分隔符
bool isSeparater(char ch)
{
	if (ch == ';')
		return true;
	else if (ch == ',')
		return true;
	else if (ch == '{')
		return true;
	else if (ch == '}')
		return true;
	else if (ch == '[')
		return true;
	else if (ch == ']')
		return true;
	else if (ch == '(')
		return true;
	else if (ch == ')')
		return true;
	else if (ch == '.')
		return true;
	else if (ch == '\'')
		return true;
	else if (ch == ':')
		return true;
	else
		return false;
}

// output
// 输出到文件
void printTokenList()
{
	ofstream output(OUT1);

	int i = 0;
	while (tokenList[i].wd.tok != ENDFILE)
	{
		output << tokenList[i].line
			<< "\t" << std::left << setw(16) << getString(tokenList[i].wd.tok)
			<< "\t" << tokenList[i].wd.str << endl;

		cout << tokenList[i].line
			<< "\t" << std::left << setw(16) << getString(tokenList[i].wd.tok)
			<< "\t" << tokenList[i].wd.str << endl;

		i++;
	}
	output << tokenList[i].line
		<< "\t" << std::left << setw(16) << getString(tokenList[i].wd.tok)
		<< "\t" << tokenList[i].wd.str << endl;

	cout << tokenList[i].line
		<< "\t" << std::left << setw(16) << getString(tokenList[i].wd.tok)
		<< "\t" << tokenList[i].wd.str << endl;

	output.close();
}

// 把lex从数字转成字符串
string getString(int lexNum)
{
	if (lexNum == 0)
		return "ENDFILE";
	else if (lexNum == 1)
		return "ERROR";
	/*reserved word*/
	else if (lexNum == 2)
		return "PROGRAM";
	else if (lexNum == 3)
		return "PROCEDURE";
	else if (lexNum == 4)
		return "TYPE";
	else if (lexNum == 5)
		return "VAR";
	else if (lexNum == 6)
		return "IF";
	else if (lexNum == 7)
		return "THEN";
	else if (lexNum == 8)
		return "ELSE";
	else if (lexNum == 9)
		return "FI";
	else if (lexNum == 10)
		return "WHILE";
	else if (lexNum == 11)
		return "DO";
	else if (lexNum == 12)
		return "ENDWH";
	else if (lexNum == 13)
		return "BEGIN";
	else if (lexNum == 14)
		return "END";
	else if (lexNum == 15)
		return "READ";
	else if (lexNum == 16)
		return "WRITE";
	else if (lexNum == 17)
		return "ARRAY";
	else if (lexNum == 18)
		return "OF";
	else if (lexNum == 19)
		return "RECORD";
	else if (lexNum == 20)
		return "RETURN";
	else if (lexNum == 21)
		return "INTEGER";
	else if (lexNum == 22)
		return "CHAR";
	/*multi char*/
	else if (lexNum == 23)
		return "ID";
	else if (lexNum == 24)
		return "INTC";
	else if (lexNum == 25)
		return "CHARC";
	/*special*/
	else if (lexNum == 26)
		return "ASSIGN";
	else if (lexNum == 27)
		return "EQ";
	else if (lexNum == 28)
		return "LT";
	else if (lexNum == 29)
		return "PLUS";
	else if (lexNum == 30)
		return "MINUS";
	else if (lexNum == 31)
		return "TIMES";
	else if (lexNum == 32)
		return "OVER";
	else if (lexNum == 33)
		return "LPAREN";
	else if (lexNum == 34)
		return "RPAREN";
	else if (lexNum == 35)
		return "DOT";
	else if (lexNum == 36)
		return "COLON";
	else if (lexNum == 37)
		return "SEMI";
	else if (lexNum == 38)
		return "COMMA";
	else if (lexNum == 39)
		return "LMIDPAREN";
	else if (lexNum == 40)
		return "RMIDPAREN";
	else if (lexNum == 41)
		return "UNDERANGE";
}

// Lexical analyse
// 词法分析
void lexicalAnalyse(FILE *fp)
{
	int lineNum = 1; // line number, start from 1
	// 记录行号 在后面写token的时候进行记录

	int index = 0; // tokenList array index
	// 用于token的记录

	char ch = fgetc(fp); // f get char
	// 初始化 取出第一个char

	string record = ""; // 当收集到char 存入string record

	// 当遇到 end of file 时暂停
	while (ch != EOF)
	{

		// letter
		// 检查是不是字母
		if (isLetter(ch))
		{
			record = ""; // 用来记录字符串
			record = record + ch; // 先把第一个字符存进来


			////////////////取字符串开始////////////////
			ch = fgetc(fp); // 把第二个字符取出来 与上面类似 
			while (isLetter(ch) || isDigit(ch)) // 注意 这里无论是字母或数字都可以保存
			{
				record = record + ch; // 存进字符串
				ch = fgetc(fp); // 循环 取出下一个字符
				// 注意 别忘了取出下一个
			}
			////////////////取字符串结束////////////////
			// 目前 record中已经保存有完整的字符串

			/*
				INID标识符状态
			*/
			// 字符串 有可能是ID 也有可能是reversedWord
			if (isReserved(record)) // 检查是ID 还是保留字
			{
				// 是保留字
				tokenList[index].line = lineNum; // 记录行号
				tokenList[index].wd.str = getReserved(record).str; // 记录字符串
				tokenList[index].wd.tok = getReserved(record).tok; // 记录token
				index++;
			}
			else
			{
				// 是ID
				tokenList[index].line = lineNum; // 记录行号
				tokenList[index].wd.str = record; // 记录字符串
				tokenList[index].wd.tok = ID; // 记录token
				index++;
			}
		}

		// int
		// 检查是不是数字
		else if (isDigit(ch))
		{
			record = ""; // 同上 用于记录
			record += ch; //


			////////////////取字符串开始////////////////
			ch = fgetc(fp); // 把第一个取出来

			while (isDigit(ch) || isLetter(ch))
			{
				record += ch;
				ch = fgetc(fp);
			}
			////////////////取字符串结束////////////////
			// 目前 record中已经保存有完整的字符串


			int flag = 1; // all digit? 目前的版本中 只允许全部为数字的
			for (int i = 0; i < record.length(); i++)
			{
				if (isLetter(record[i])) // 只要有字母 就退出
				{
					flag = 0;
					tokenList[index].wd.tok = ERROR;
					break;
				}
			}
			/*
				INNUM数字状态
			*/
			if (flag == 1) // 全部为数字 记作INTC
				tokenList[index].wd.tok = INTC;

			tokenList[index].line = lineNum;
			tokenList[index].wd.str = record;
			index++;
		}

		// operator
		// 运算符检测
		else if (isOperator(ch)) // 检查是不是运算符
		{
			string temp = ""; // char转string
			temp = temp + ch;

			if (temp == "+")
				tokenList[index].wd.tok = PLUS;
			else if (temp == "-")
				tokenList[index].wd.tok = MINUS;
			else if (temp == "*")
				tokenList[index].wd.tok = TIMES;
			else if (temp == "/")
				tokenList[index].wd.tok = OVER;
			else if (temp == "<")
				tokenList[index].wd.tok = LT;
			else if (temp == "=")
				tokenList[index].wd.tok = EQ;

			tokenList[index].line = lineNum;
			tokenList[index].wd.str = temp;

			index++;
			ch = fgetc(fp);
		}

		// separater
		else if (isSeparater(ch)) // 检查是不是分隔符
		{
			record = "";
			// ignore annotation
			// {}注释 忽略掉
			/*
				INCOMMENT注释状态
			*/
			if (ch == '{')
			{
				while (ch != '}')
				{
					ch = fgetc(fp);
					if (ch == '\n')
						lineNum += 1;
				}
				ch = fgetc(fp);
			}
			// limit operator
			// 
			else if (ch == '.')
			{
				record += ch;
				if ((ch = fgetc(fp)) == '.') // 记录'..'
				{
					/*
						INRANGE数组下标界限状态
					*/
					record += ch;
					tokenList[index].line = lineNum;
					tokenList[index].wd.str = record;
					tokenList[index].wd.tok = UNDERANGE;
					index++;
					ch = fgetc(fp);
				}
				else // 记录'.'
				{
					tokenList[index].line = lineNum;
					tokenList[index].wd.str = record;
					tokenList[index].wd.tok = DOT;
					index++;
				}
			}
			// string
			/*
				INCHAR字符标志状态
			*/
			else if (ch == '\'')
			{
				tokenList[index].line = lineNum;
				string temp = "";
				temp = temp + ch;
				tokenList[index].wd.tok = CHARC;
				while ((ch = fgetc(fp)) != '\'')
				{
					record += ch;
				}
				tokenList[index].wd.str = record;
				index++;
				ch = fgetc(fp);
			}
			// :
			else if (ch == ':')
			{
				record += ch;
				if ((ch = fgetc(fp)) == '=')
				{
					/*
						INASSIGN赋值状态
					*/
					record += ch;
					tokenList[index].line = lineNum;
					tokenList[index].wd.str = record;
					tokenList[index].wd.tok = ASSIGN;
					index++;
					ch = fgetc(fp);
				}
				else
				{
					tokenList[index].line = lineNum;
					tokenList[index].wd.str = record;
					tokenList[index].wd.tok = COLON;
					index++;

				}
			}
			//
			else
			{
				tokenList[index].line = lineNum;
				string temp = "";
				temp = temp + ch;
				tokenList[index].wd.str = temp;

				if (temp == "(")
					tokenList[index].wd.tok = LPAREN;
				else if (temp == ")")
					tokenList[index].wd.tok = RPAREN;
				else if (temp == "[")
					tokenList[index].wd.tok = LMIDPAREN;
				else if (temp == "]")
					tokenList[index].wd.tok = RMIDPAREN;
				else if (temp == ";")
					tokenList[index].wd.tok = SEMI;
				else if (temp == ",")
					tokenList[index].wd.tok = COMMA;

				index++;
				ch = fgetc(fp);
			}
		}

		// blank
		// 检查是不是 blank
		// 空格 制表符 回车 换行
		else if (isBlank(ch))
		{
			if (ch == '\n') // 换行符特殊处理 行号+1
				lineNum += 1;
			else
				;
			ch = fgetc(fp);
		}

		// unknown
		else
		{
			string t = "Line ";
			t += to_string(lineNum);
			t += " ";
			t += "\"";
			t += ch;
			t += "\" ";
			t += "UnknownToken Error!\n";
			printErrorMsg(t);
		}
	}
	/*
		DONE完成状态
	*/
	tokenList[index].line = lineNum;
	tokenList[index].wd.str = ch;
	tokenList[index].wd.tok = ENDFILE;
}

/****************************************************************/
/****************************************************************/

void readToken()
{
	currentToken = &tokenList[INDEX];
	//cout << currentToken->wd.str;
	INDEX++;
}

TreeNode* matchToken(LexType tok)
{
	readToken();

	if (currentToken->wd.tok == tok)
	{

		TreeNode* t = new TreeNode(currentToken->wd.str);
		t->tk = currentToken;
		return t;
	}
	else
		return NULL;
}

// 错误输出函数
void printErrorMsg(string reason)
{
	string temp;
	temp += reason;
	temp += '\n';
	ofstream output(OUTerror);
	output << temp;
	cout << temp;
	output.close();
	exit(0);
}

// 下面的部分是递归下降方法建立语法树
// 原理非常简单所以就不写什么注释了
// 总结一下就是如果没有多种选择就直接生成子树
// 如果有多种选择就检查下currentToken来判断该选择哪种生成方式
///*
/* Program ::= ProgramHead DeclarePart ProgramBody*/
TreeNode* program()
{
	INDEX = 0;
	TreeNode* t = new TreeNode("Program");
	t->addChild(programHead());
	t->addChild(declarePart());
	t->addChild(programBody());
	return t;
}
/*ProgramHead ::= PROGRAM ProgramName
ProgramName ::= ID*/
TreeNode* programHead()
{
	TreeNode* t = new TreeNode("ProgramHead");
	t->addChild(matchToken(PROGRAM));
	t->addChild(matchToken(ID));
	return t;
}
/* DeclarePart ::= TypeDecpart VarDecpart ProcDecpart */
TreeNode* declarePart()
{
	TreeNode* t = new TreeNode("DeclarePart");
	t->addChild(typeDecPart());
	t->addChild(varDecPart());
	t->addChild(procDecpart());
	return t;
}
/*ProgramBody ::= BEGIN StmList END*/
TreeNode* programBody()
{
	TreeNode* t = new TreeNode("ProgramBody");
	t->addChild(matchToken(BEGIN));
	t->addChild(stmList());
	t->addChild(matchToken(END1));
	return t;
}
/*TypeDecpart ::= ε
							| TypeDec */
TreeNode* typeDecPart()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == TYPE)
	{
		t = new TreeNode("TypeDecPart");
		t->addChild(typeDec());
	}
	return t;
}
/* VarDecpart ::= ε
						 | VarDec*/
TreeNode* varDecPart()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == VAR)
	{
		t = new TreeNode("VarDecPart");
		t->addChild(varDec());
	}
	return t;
}
/* ProcDecpart ::= ε
							 | ProcDec*/
TreeNode* procDecpart()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == PROCEDURE)
	{
		t = new TreeNode("ProcDecPart");
		t->addChild(procDec());
	}
	return t;
}
/* TypeDec ::= TYPE TypeDecList */
TreeNode* typeDec()
{
	TreeNode* t = new TreeNode("TypeDec");
	t->addChild(matchToken(TYPE));
	t->addChild(typeDecList());
	return t;
}
/*TypeDecList ::= TypeId = TypeDef; TypeDecMore*/
TreeNode* typeDecList()
{
	TreeNode* t = new TreeNode("TypeDecList");
	//t->addChild(typeID());
	t->addChild(matchToken(ID));
	t->addChild(matchToken(EQ));
	t->addChild(typeDef());
	t->addChild(matchToken(SEMI));
	t->addChild(typeDecMore());
	return t;
}
/* TypeId ::= ID */
/*
TreeNode* typeID()
{
	TreeNode* t = new TreeNode("TypeID");
	t->addChild(matchToken(ID));
	return t;
}
//*/
/*TypeDef ::= BaseType
					 | StructureType
					 | ID*/
TreeNode* typeDef()
{
	TreeNode* t = new TreeNode("TypeDef");
	if ((tokenList[INDEX].wd.tok == INTEGER) ||
		(tokenList[INDEX].wd.tok == CHAR1))
	{
		t->addChild(baseType());
		return t;
	}
	else if ((tokenList[INDEX].wd.tok == ARRAY) ||
		(tokenList[INDEX].wd.tok == RECORD))
	{
		t->addChild(structureType());
		return t;
	}
	else
	{
		t->addChild(matchToken(ID));
		return t;
	}
}
/* TypeDecMore ::= ε
							 | TypeDecList */
TreeNode* typeDecMore()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == ID)
	{
		t = new TreeNode("TypeDecMore");
		t->addChild(typeDecList());
	}

	return t;
}
/*BaseType ::= INTEGER
					 | CHAR */
TreeNode* baseType()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == INTEGER)
	{
		t = new TreeNode("BaseType");
		t->addChild(matchToken(INTEGER));
		return t;
	}
	else if (tokenList[INDEX].wd.tok == CHAR1)
	{
		t = new TreeNode("BaseType");
		t->addChild(matchToken(CHAR1));
		return t;
	}

	readToken();
	string temp = "Line ";
	temp += to_string(currentToken->line);
	temp += " ";
	temp += "\"";
	temp += currentToken->wd.str;
	temp += "\" ";
	temp += "BaseType Error!\n";
	printErrorMsg(temp);
	return t;
}
/* StructureType ::= ArrayType
							 | RecType */
TreeNode* structureType()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == ARRAY)
	{
		t = new TreeNode("StructureType");
		t->addChild(arrayType());
		return t;
	}
	if (tokenList[INDEX].wd.tok == RECORD)
	{
		t = new TreeNode("StructureType");
		t->addChild(recType());
		return t;
	}
	return t;
}
/* ArrayType ::= ARRAY [low..top ] OF BaseType */
TreeNode* arrayType()
{
	TreeNode* t = new TreeNode("ArrayType");
	t->addChild(matchToken(ARRAY));
	t->addChild(matchToken(LMIDPAREN));
	t->addChild(matchToken(INTC));
	t->addChild(matchToken(UNDERANGE));
	t->addChild(matchToken(INTC));
	t->addChild(matchToken(RMIDPAREN));
	t->addChild(matchToken(OF));
	t->addChild(baseType());
	//t->addChild(matchToken(SEMI));
	return t;
}
/* RecType ::= RECORD FieldDecList END*/
TreeNode* recType()
{
	TreeNode* t = new TreeNode("RecType");
	t->addChild(matchToken(RECORD));
	t->addChild(fieldDecList());
	t->addChild(matchToken(END1));
	return t;
}
/* FieldDecList ::= BaseType IdList ; FieldDecMore
						 | ArrayType IdList ; FieldDecMore*/
TreeNode* fieldDecList()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == INTEGER ||
		tokenList[INDEX].wd.tok == CHAR1)
	{
		t = new TreeNode("FieldDecList");
		t->addChild(baseType());
		t->addChild(IDList());
		t->addChild(matchToken(SEMI));
		t->addChild(fieldDecMore());
		return t;
	}
	else if (tokenList[INDEX].wd.tok == ARRAY)
	{
		t = new TreeNode("FieldDecList");
		t->addChild(arrayType());
		t->addChild(IDList());
		t->addChild(matchToken(SEMI));
		t->addChild(fieldDecMore());
		return t;
	}
	readToken();
	string temp = "Line ";
	temp += to_string(currentToken->line);
	temp += " ";
	temp += "FieldDeclareList Error!\n";
	printErrorMsg(temp);
	return t;
}
/* IdList ::= ID IdMore*/
TreeNode* IDList()
{
	TreeNode* t = new TreeNode("IDList");
	t->addChild(matchToken(ID));
	t->addChild(IDMore());
	return t;
}
/* FieldDecMore ::= ε
							 | FieldDecList */
TreeNode* fieldDecMore()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == INTEGER ||
		tokenList[INDEX].wd.tok == CHAR1 ||
		tokenList[INDEX].wd.tok == ARRAY)
	{
		t = new TreeNode("FieldDecMore");
		t->addChild(fieldDecList());
	}
	return t;
}
/* IdMore ::= ε
					| , IdList */
TreeNode* IDMore()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == COMMA) {
		t = new TreeNode("IDMore");
		t->addChild(matchToken(COMMA));
		t->addChild(IDList());
	}
	return t;
}
/* VarDec ::= VAR VarDecList */
TreeNode* varDec()
{
	TreeNode* t = new TreeNode("VarDec");
	t->addChild(matchToken(VAR));
	t->addChild(varDecList());
	return t;
}
/* VarDecList ::= TypeDef VarIdList ; VarDecMore*/
TreeNode* varDecList()
{
	TreeNode* t = new TreeNode("VarDecList");
	t->addChild(typeDef());
	t->addChild(varIDList());
	t->addChild(matchToken(SEMI));
	t->addChild(varDecMore());
	return t;
}
/*VarIdList ::= ID VarIdMore*/
TreeNode* varIDList()
{
	TreeNode* t = new TreeNode("VarIDList");
	t->addChild(matchToken(ID));
	t->addChild(varIDMore());
	return t;
}
/* VarDecMore ::= ε
							 | VarDecList*/
TreeNode* varDecMore()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == INTEGER ||
		tokenList[INDEX].wd.tok == CHAR1 ||
		tokenList[INDEX].wd.tok == ARRAY ||
		tokenList[INDEX].wd.tok == RECORD ||
		tokenList[INDEX].wd.tok == ID)
	{
		t = new TreeNode("VarDecMore");
		t->addChild(varDecList());
	}
	return t;
}
/*VarIdMore ::= ε
						 | , VarIdList */
TreeNode* varIDMore()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == COMMA)
	{
		t = new TreeNode("VarIDMore");
		t->addChild(matchToken(COMMA));
		t->addChild(varIDList());
	}
	return t;
}
/*ProcDec ::= PROCEDURE
ProcName ( ParamList ) ;
ProcDecPart
ProcBody
ProcDecMore*/
TreeNode* procDec()
{
	TreeNode* t = new TreeNode("ProcDec");
	t->addChild(matchToken(PROCEDURE));
	t->addChild(matchToken(ID));
	t->addChild(matchToken(LPAREN));
	t->addChild(paramList());
	t->addChild(matchToken(RPAREN));
	t->addChild(matchToken(SEMI));
	t->addChild(procDecPart());
	t->addChild(procBody());
	t->addChild(procDecpart());
	return t;
}
/*ParamList ::= ε
					 | ParamDecList*/
TreeNode* paramList()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == INTEGER ||
		tokenList[INDEX].wd.tok == CHAR1 ||
		tokenList[INDEX].wd.tok == ARRAY ||
		tokenList[INDEX].wd.tok == RECORD ||
		tokenList[INDEX].wd.tok == VAR ||
		tokenList[INDEX].wd.tok == ID)
	{
		t = new TreeNode("ParamList");
		t->addChild(paramDecList());
	}
	return t;
}
/* ProcDecPart ::= DeclarePart */
TreeNode* procDecPart()
{
	TreeNode* t = new TreeNode("ProcDecPart");
	t->addChild(declarePart());
	return t;
}
/*ProcBody ::= ProgramBody*/
TreeNode* procBody()
{
	TreeNode* t = new TreeNode("ProcBody");
	t->addChild(programBody());
	return t;
}
/* ParamList ::= ε
						 | ParamDecList*/
TreeNode* paramDecList()
{
	TreeNode* t = new TreeNode("ParamDecList");
	t->addChild(param());
	t->addChild(paramMore());
	return t;
}
/* Param ::= TypeDef FormList
				 | VAR TypeDef FormList */
TreeNode* param()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == INTEGER ||
		tokenList[INDEX].wd.tok == CHAR1 ||
		tokenList[INDEX].wd.tok == ARRAY ||
		tokenList[INDEX].wd.tok == RECORD ||
		tokenList[INDEX].wd.tok == ID)
	{
		t = new TreeNode("Param");
		t->addChild(typeDef());
		t->addChild(formList());
		return t;
	}
	else if (tokenList[INDEX].wd.tok == VAR)
	{
		t = new TreeNode("Param");
		t->addChild(matchToken(VAR));
		t->addChild(typeDef());
		t->addChild(formList());
		return t;
	}
	readToken();
	string temp = "Line ";
	temp += to_string(currentToken->line);
	temp += " ";
	temp += "\"";
	temp += currentToken->wd.str;
	temp += "\" ";
	temp += "Param error\n";
	printErrorMsg(temp);
	return t;
}
/* ParamMore ::= ε
						 | ; ParamDecList*/
TreeNode* paramMore()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == SEMI)
	{
		t = new TreeNode("ParamMore");
		t->addChild(matchToken(SEMI));
		t->addChild(paramDecList());
	}
	return t;
}
/* FormList ::= ID FidMore */
TreeNode* formList()
{
	TreeNode* t = new TreeNode("FormList");
	t->addChild(matchToken(ID));
	t->addChild(fidMore());
	return t;
}
/* FidMore ::= ε
					 | , FormList */
TreeNode* fidMore()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == COMMA)
	{
		t = new TreeNode("FidMore");
		t->addChild(matchToken(COMMA));
		t->addChild(formList());
	}
	return t;
}
/* StmList ::= Stm StmMore*/
TreeNode* stmList()
{
	TreeNode* t = new TreeNode("StmList");
	t->addChild(stm());
	t->addChild(stmMore());
	return t;
}
/* Stm ::= ConditionalStm
			 | LoopStm
			 | InputStm
			 | OutputStm
			 | ReturnStm
			 | ID AssCall */
TreeNode* stm()
{
	TreeNode* t = new TreeNode("Stm");
	if (tokenList[INDEX].wd.tok == IF) {
		t->addChild(conditionalStm());
		return t;
	}
	else if (tokenList[INDEX].wd.tok == WHILE)
	{
		t->addChild(loopStm());
		return t;
	}
	else if (tokenList[INDEX].wd.tok == READ)
	{
		t->addChild(inputStm());
		return t;
	}
	else if (tokenList[INDEX].wd.tok == WRITE)
	{
		t->addChild(outputStm());
		return t;
	}
	else if (tokenList[INDEX].wd.tok == RETURN1)
	{
		t->addChild(returnStm());
		return t;
	}
	else if (tokenList[INDEX].wd.tok == ID)
	{
		t->addChild(matchToken(ID));
		t->addChild(assCall());
		return t;
	}
	return NULL;
}
/* StmMore ::= ε
					 | ; StmList*/
TreeNode* stmMore()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == SEMI) {
		t = new TreeNode("StmMore");
		t->addChild(matchToken(SEMI));
		t->addChild(stmList());
	}
	return t;
}
/*ConditionalStm ::= IF RelExp THEN StmList ELSE StmList FI*/
TreeNode* conditionalStm()
{
	TreeNode* t = new TreeNode("ConditionalStm");
	t->addChild(matchToken(IF));
	t->addChild(exp());
	if (tokenList[INDEX].wd.tok == LT)
	{
		t->addChild(matchToken(LT));
	}
	else if (tokenList[INDEX].wd.tok == EQ)
	{
		t->addChild(matchToken(EQ));
	}
	else
	{
		readToken();
		string temp = "Line ";
		temp += to_string(currentToken->line);
		temp += " ";
		temp += "\"";
		temp += currentToken->wd.str;
		temp += "\" ";
		temp += "ConditionalStm Error!\n";
		printErrorMsg(temp);
	}
	t->addChild(exp());
	t->addChild(matchToken(THEN));
	t->addChild(stmList());
	t->addChild(matchToken(ELSE));
	t->addChild(stmList());
	t->addChild(matchToken(FI));
	return t;
}
/* LoopStm ::= WHILE RelExp DO StmList ENDWH*/
TreeNode* loopStm()
{
	TreeNode* t = new TreeNode("LoopStm");
	t->addChild(matchToken(WHILE));
	t->addChild(exp());
	if (tokenList[INDEX].wd.tok == LT)
	{
		t->addChild(matchToken(LT));
	}
	else if (tokenList[INDEX].wd.tok == EQ)
	{
		t->addChild(matchToken(EQ));
	}
	else
	{
		readToken();
		string temp = "Line ";
		temp += to_string(currentToken->line);
		temp += " ";
		temp += "\"";
		temp += currentToken->wd.str;
		temp += "\" ";
		temp += "LoopStm Error!\n";
		printErrorMsg(temp);
	}
	t->addChild(exp());
	t->addChild(matchToken(DO));
	t->addChild(stmList());
	t->addChild(matchToken(ENDWH));
	return t;
}
/* InputStm ::= READ ( Invar)*/
TreeNode* inputStm()
{
	TreeNode* t = new TreeNode("InputStm");
	t->addChild(matchToken(READ));
	t->addChild(matchToken(LPAREN));
	t->addChild(matchToken(ID));
	t->addChild(matchToken(RPAREN));
	return t;
}
/* OutputStm ::= WRITE( Exp )*/
TreeNode* outputStm()
{
	TreeNode* t = new TreeNode("OutputStm");
	t->addChild(matchToken(WRITE));
	t->addChild(matchToken(LPAREN));
	t->addChild(exp());
	t->addChild(matchToken(RPAREN));
	return t;
}
/* ReturnStm ::= RETURN*/
TreeNode* returnStm()
{
	TreeNode* t = new TreeNode("ReturnStm");
	t->addChild(matchToken(RETURN1));
	return t;
}
/*AssCall ::= AssignmentRest
| CallStmRest */
TreeNode* assCall()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == LMIDPAREN ||
		tokenList[INDEX].wd.tok == DOT ||
		tokenList[INDEX].wd.tok == ASSIGN)
	{
		t = new TreeNode("AssCall");
		t->addChild(assignmentRest());
		return t;
	}
	else if (tokenList[INDEX].wd.tok == LPAREN)
	{
		t = new TreeNode("AssCall");
		t->addChild(callStmRest());
		return t;
	}

	string temp = "Line ";
	temp += to_string(currentToken->line);
	temp += " ";
	temp += "\"";
	temp += currentToken->wd.str;
	temp += "\" ";
	temp += "AssCall Error!\n";
	printErrorMsg(temp);
	return t;
}
/* AssignmentRest ::= VariMore := Exp */
TreeNode* assignmentRest()
{
	TreeNode* t = new TreeNode("AssignmentRest");
	if (tokenList[INDEX].wd.tok == LMIDPAREN ||
		tokenList[INDEX].wd.tok == DOT)
	{
		t->addChild(variMore());
	}
	t->addChild(matchToken(COLON));
	//t->addChild(matchTokenChar('='));
	t->addChild(exp());
	return t;
}
/* CallStmRest ::= ( ActParamList ) */
TreeNode* callStmRest()
{
	TreeNode* t = new TreeNode("CallStmRest");
	t->addChild(matchToken(LPAREN));
	t->addChild(actparamList());
	t->addChild(matchToken(RPAREN));
	return t;
}
/* VariMore ::= ε
					 | [ Exp ] */
TreeNode* variMore()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == LMIDPAREN)
	{
		t = new TreeNode("VariMore");
		t->addChild(matchToken(LMIDPAREN));
		t->addChild(exp());
		t->addChild(matchToken(RMIDPAREN));
	}
	else if (tokenList[INDEX].wd.tok == DOT)
	{
		t = new TreeNode("VariMore");
		t->addChild(matchToken(DOT));
		t->addChild(fieldVar());
	}
	return t;
}
/* Exp ::= Term OtherTerm*/
TreeNode*exp() {
	TreeNode* t = new TreeNode("Exp");
	t->addChild(term());
	t->addChild(otherTerm());
	return t;
}
/* ActParamList ::= ε  | Exp ActParamMore*/
TreeNode* actparamList()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == LPAREN ||
		tokenList[INDEX].wd.tok == INTC ||
		tokenList[INDEX].wd.tok == ID)
	{
		t = new TreeNode("ActParamList");
		t->addChild(exp());
		t->addChild(actparamMore());
	}
	return t;
}
/* ActParamMore ::= ε
							 | , ActParamList */
TreeNode* actparamMore()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == COMMA)
	{
		t = new TreeNode("ActParamMore");
		t->addChild(matchToken(COMMA));
		t->addChild(actparamList());
	}
	return t;
}
/* Term ::= Factor OtherFactor*/
TreeNode* term()
{
	TreeNode* t = new TreeNode("Term");
	t->addChild(factor());
	t->addChild(otherFactor());
	return t;
}
/*OtherFactor ::= ε
						 | MultOpTerm*/
TreeNode* otherTerm()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == PLUS ||
		tokenList[INDEX].wd.tok == MINUS) {
		TreeNode* t = new TreeNode("OtherTerm");
		t->addChild(addOp());
		t->addChild(exp());
	}
	return t;
}
/*AddOp ::= +  | - */
TreeNode* addOp()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == PLUS)
	{
		TreeNode* t = new TreeNode("AddOp");
		t->addChild(matchToken(PLUS));
		return t;
	}
	else if (tokenList[INDEX].wd.tok == MINUS)
	{
		TreeNode* t = new TreeNode("AddOp");
		t->addChild(matchToken(MINUS));
		return t;
	}
	//printErrorMsg("cmpop error\n");
	string temp = "Line ";
	temp += to_string(currentToken->line);
	temp += " ";
	temp += "\"";
	temp += currentToken->wd.str;
	temp += "\" ";
	temp += "CompareOp Error!\n";
	printErrorMsg(temp);
	return t;
}
/* Factor ::= ( Exp )
				 | INTC
				 | Variable*/
TreeNode* factor()
{
	TreeNode* t = new TreeNode("Factor");


	if (tokenList[INDEX].wd.tok == INTC)
	{
		t->addChild(matchToken(INTC));
		return t;
	}
	else if (tokenList[INDEX].wd.tok == LPAREN)
	{
		t->addChild(matchToken(LPAREN));
		t->addChild(exp());
		t->addChild(matchToken(RPAREN));
		return t;
	}
	else if (tokenList[INDEX].wd.tok == ID)
	{
		t->addChild(variable());
		return t;
	}
	string temp = "Line ";
	temp += to_string(currentToken->line);
	temp += " ";
	temp += "\"";
	temp += currentToken->wd.str;
	temp += "\" ";
	temp += "Factor Error!\n";
	printErrorMsg(temp);
	//printErrorMsg("factor error\n");
	return NULL;
}
/* OtherFactor ::= ε| MultOp Term*/
TreeNode* otherFactor()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == TIMES ||
		tokenList[INDEX].wd.tok == OVER)
	{
		TreeNode* t = new TreeNode("OtherFactor");
		t->addChild(multOp());
		t->addChild(term());
	}
	return t;
}
/*MultOp ::= *  | /	*/
TreeNode* multOp()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == TIMES)
	{
		TreeNode* t = new TreeNode("MultOp");
		t->addChild(matchToken(TIMES));
		return t;
	}
	else if (tokenList[INDEX].wd.tok == OVER)
	{
		TreeNode* t = new TreeNode("MultOp");
		t->addChild(matchToken(OVER));
		return t;
	}
	//printErrorMsg("cmpop error\n");
	string temp = "Line ";
	temp += to_string(currentToken->line);
	temp += " ";
	temp += "\"";
	temp += currentToken->wd.str;
	temp += "\" ";
	temp += "CompareOp Error!\n";
	printErrorMsg(temp);
	return t;
}
/* Variable ::= ID VariMore*/
TreeNode* variable()
{
	TreeNode* t = new TreeNode("Variable");
	t->addChild(matchToken(ID));
	t->addChild(variMore());
	return t;
}
/*FieldVar ::= ID FieldVarMore*/
TreeNode* fieldVar()
{
	TreeNode* t = new TreeNode("FieldVar");
	t->addChild(matchToken(ID));
	t->addChild(fieldVarMore());
	return t;
}
/* FieldVarMore ::= ε
 | [ Exp ] */
TreeNode* fieldVarMore()
{
	TreeNode* t = NULL;
	if (tokenList[INDEX].wd.tok == LMIDPAREN)
	{
		TreeNode* t = new TreeNode("FieldVarMore");
		t->addChild(matchToken(LMIDPAREN));
		t->addChild(exp());
		t->addChild(matchToken(RMIDPAREN));

	}
	return t;
}

// 前序遍历递归打印树
// 这是我最喜欢的函数
void printTree(TreeNode * t, int layer, bool islastson, char out[100])
{
	if (t == NULL)
		return;// 递归出口

	int last = 31; //这段代码的目的是找到当前结点最后一个儿子的index
	while (last >= 0)
	{
		if (t->child[last] != NULL)
			break;
		// 找到最后一个儿子节点
		last--;
	}
	// 如果没有儿子 last == -1
	// 否则 last 记录最后一个儿子节点的index


	for (int i = 0; i < layer; i++) //根据层数打印空格
	{
		outstr[strline] += "   ";
		//printf("   ");
	}

	if (layer == 0) //特殊情况 根节点的显示
	{
		outstr[strline] += "   ";
		outstr[strline] += t->name;
		//cout << t->name;
	}
	else //不是根节点的时候
	{
		if (islastson == true) // 如果当前节点是父节点的最后一个儿子节点
		{
			outstr[strline] += "└ "; // 则它前面的树枝会有些区别
			outstr[strline] += t->name;
			//cout << "└ " << t->name;
		}
		else// 不是父节点的最后一个儿子节点
		{
			outstr[strline] += "├ ";
			outstr[strline] += t->name;
			//cout << "├ " << t->name;
		}
	}

	outstr[strline] += "\n"; //换行
	strline++;
	//outstr += "\n";
	//cout << endl;

	for (int i = 0; i < 10; i++) //递归
	{
		if (i != last) // 如果不是儿子节点 那么第三个参数就标成false
			printTree(t->child[i], layer + 1, false, out);
		else
			printTree(t->child[i], layer + 1, true, out);
	}

	if (layer == 0) // 在整个递归过程结束退出之前进行打印
	{
		// 首先对字符串进行修改
		// 这里修改的目的是让树枝能连起来
		// 底下这段代码的功能比较难以描述了
		// 你可以注释掉一些部分自己看一看会对打印的树有什么影响
		///*
		for (int i = 1; i < strline; i++)
		{
			int j = 0;
			while (outstr[i][j] != '\n')
			{
				if (outstr[i - 1][j] == '\n')
					break;
				if (outstr[i - 1][j] == ch2[0] && outstr[i - 1][j + 1] == ch2[1]) // 这个很重要 不加的话会出问题
				{
					; // 上一行是"└"
				}
				else if (outstr[i - 1][j] == ch1[0] && outstr[i - 1][j + 1] == ch1[1] && outstr[i][j] == ' ')
				{
					// 上一行是"├" 这一行是空格
					outstr[i][j] = ch3[0];
					outstr[i][j + 1] = ch3[1];
				}
				else if (outstr[i - 1][j] == ch3[0] && outstr[i][j] == ' ')
				{
					// 上一行是"│" 这一行是空格
					outstr[i][j] = ch3[0];
					outstr[i][j + 1] = ch3[1];
				}
				j++;
			}
		}
		//*/

		// 修改完之后进行打印
		ofstream output(out);
		for (int i = 0; i < strline; i++)
		{
			output << outstr[i];
		}
		output.close();
		for (int i = 0; i < strline; i++)
		{
			cout << outstr[i];
		}
	}
}

// 与printTree没有本质上的区别 因为string在多次覆写之后出现了乱码 故将两次输出拆分成两个函数
// 打印LL1树时如果行数比较多可能会有乱码影响美观 由于时间紧张暂时未能找到问题 我怀疑可能是全角字符导致的
void printTreeLL1(TreeNode * t, int layer, bool islastson, char out[100])
{
	if (t == NULL)
		return;

	int last = 31;
	while (last >= 0)
	{
		if (t->child[last] != NULL)
			break;
		// 找到最后一个儿子节点
		last--;
	}
	// 如果没有儿子 last == -1
	// 否则last指向最后一个


	for (int i = 0; i < layer; i++)
	{
		outstrLL1[strlineLL1] += "   ";
	}

	if (layer == 0)
	{
		outstrLL1[strlineLL1] += "   ";
		outstrLL1[strlineLL1] += t->name;
		//cout << t->name;
	}
	else
	{
		if (islastson == true)
		{
			outstrLL1[strlineLL1] += "└ ";
			outstrLL1[strlineLL1] += t->name;
			//cout << "└ " << t->name;
		}
		else
		{
			outstrLL1[strlineLL1] += "├ ";
			outstrLL1[strlineLL1] += t->name;
			//cout << "├ " << t->name;
		}
	}

	outstrLL1[strlineLL1] += "\n";
	strlineLL1++;
	//outstr += "\n";
	//cout << endl;

	for (int i = 0; i < 10; i++)
	{
		if (i != last)
			printTreeLL1(t->child[i], layer + 1, false, out);
		else
			printTreeLL1(t->child[i], layer + 1, true, out);
	}

	if (layer == 0)
	{
		// 首先对字符串进行修改
		///*
		for (int i = 1; i < strlineLL1; i++)
		{
			int j = 0;
			while (outstrLL1[i][j] != '\n')
			{
				if (outstrLL1[i - 1][j] == '\n')
					break;
				if (outstrLL1[i - 1][j] == ch2[0] && outstrLL1[i - 1][j + 1] == ch2[1]) // 这个很重要 不加的话会出问题
				{
					; // 上一行是拐
				}
				else if (outstrLL1[i - 1][j] == ch1[0] && outstrLL1[i - 1][j + 1] == ch1[1] && outstrLL1[i][j] == ' ')
				{
					// 上一行是卜 这一行是空格
					outstrLL1[i][j] = ch3[0];
					outstrLL1[i][j + 1] = ch3[1];
				}
				else if (outstrLL1[i - 1][j] == ch3[0] && outstrLL1[i][j] == ' ')
				{
					// 上一行是棍 这一行是空格
					outstrLL1[i][j] = ch3[0];
					outstrLL1[i][j + 1] = ch3[1];
				}
				j++;
			}
		}
		//*/

		ofstream output(out);
		//char header[3] = { (char)0xEF,(char)0xBB,(char)0xBF };
		//output << header[0] << header[1] << header[2];
		// FILE*fp = fopen("C:\\Users\\Yichuan\\Desktop\\tree.txt", "wt+,ccs=UTF-8");
		for (int i = 0; i < strlineLL1; i++)
		{
			output << outstrLL1[i];
			//fwrite(&outstr[i], sizeof(char), outstr[i].length(), fp);
		}
		output.close();
		for (int i = 0; i < strlineLL1; i++)
		{
			cout << outstrLL1[i];
		}
	}
}

/****************************************************************/
/****************************************************************/

// 对process中的一部分代码进行了封装 减小一点工作量
void processAddChild(int istmnl, int lex, string name, int childindex)
{
	anlsstack->push(istmnl, lex); // 压栈 逆着压 确保最左在最上
	anlsstack->top->p = new TreeNode(name); // 新建树结点
	currentTree->child[childindex] = anlsstack->top->p; // 连接在父结点上
}

// 将书上104个process封装在一个函数中
void process(int num)
{
	if (num == 1)
	{
		int count = 4;
		processAddChild(1, DOT, "DOT", --count);
		processAddChild(0, ProgramBody, "ProgramBody", --count);
		processAddChild(0, DeclarePart, "DeclarePart", --count);
		processAddChild(0, ProgramHead, "ProgramHead", --count);
	}
	else if (num == 2)
	{
		int count = 2;
		processAddChild(0, ProgramName, "ProgramName", --count);
		processAddChild(1, PROGRAM, "PROGRAM", --count);
	}
	else if (num == 3)
	{
		int count = 1;
		//		processAddChild(1, SEMI, "SEMI", --count);
		processAddChild(1, ID, "ID", --count);
	}
	else if (num == 4)
	{
		int count = 3;
		processAddChild(0, ProcDec, "ProcDec", --count);
		processAddChild(0, VarDec, "VarDec", --count);
		processAddChild(0, TypeDec, "TypeDec", --count);
	}
	else if (num == 5)
	{
		;
	}
	else if (num == 6)
	{
		int count = 1;
		processAddChild(0, TypeDeclaration, "TypeDeclaration", --count);
	}
	else if (num == 7)
	{
		int count = 2;
		processAddChild(0, TypeDecList, "TypeDecList", --count);
		processAddChild(1, TYPE, "TYPE", --count);
	}
	else if (num == 8)
	{
		int count = 5;
		processAddChild(0, TypeDecMore, "TypeDecMore", --count);
		processAddChild(1, SEMI, "SEMI", --count);
		processAddChild(0, TypeName, "TypeName", --count);
		processAddChild(1, EQ, "EQ", --count);
		processAddChild(0, TypeId, "TypeID", --count);
	}
	else if (num == 9)
	{
		;
	}
	else if (num == 10)
	{
		int count = 1;
		processAddChild(0, TypeDecList, "TypeDecList", --count);
	}
	else if (num == 11)
	{
		int count = 1;
		processAddChild(1, ID, "ID", --count);
	}
	else if (num == 12)
	{
		int count = 1;
		processAddChild(0, BaseType, "BaseType", --count);
	}
	else if (num == 13)
	{
		int count = 1;
		processAddChild(0, StructureType, "StructureType", --count);
	}
	else if (num == 14)
	{
		int count = 1;
		processAddChild(1, ID, "ID", --count);
	}
	else if (num == 15)
	{
		int count = 1;
		processAddChild(1, INTEGER, "INTEGER", --count);
	}
	else if (num == 16)
	{
		int count = 1;
		processAddChild(1, CHAR1, "CHAR1", --count);
	}
	else if (num == 17)
	{
		int count = 1;
		processAddChild(0, ArrayType, "ArrayType", --count);
	}
	else if (num == 18)
	{
		int count = 1;
		processAddChild(0, RecType, "RecType", --count);
	}
	else if (num == 19)
	{
		int count = 8;
		processAddChild(0, BaseType, "BaseType", --count);
		processAddChild(1, OF, "OF", --count);
		processAddChild(1, RMIDPAREN, "RMIDPAREN", --count);
		processAddChild(0, Top, "Top", --count);
		processAddChild(1, UNDERANGE, "UNDERANGE", --count);
		processAddChild(0, Low, "Low", --count);
		processAddChild(1, LMIDPAREN, "LMIDPAREN", --count);
		processAddChild(1, ARRAY, "ARRAY", --count);
	}
	else if (num == 20)
	{
		int count = 1;
		processAddChild(1, INTC, "INTC", --count);
	}
	else if (num == 21)
	{
		int count = 1;
		processAddChild(1, INTC, "INTC", --count);
	}
	else if (num == 22)
	{
		int count = 3;
		processAddChild(1, END1, "END1", --count);
		processAddChild(0, FieldDecList, "FieldDecList", --count);
		processAddChild(1, RECORD, "RECORD", --count);
	}
	else if (num == 23)
	{
		int count = 4;
		processAddChild(0, FieldDecMore, "FieldDecMore", --count);
		processAddChild(1, SEMI, "SEMI", --count);
		processAddChild(0, IdList, "IDList", --count);
		processAddChild(0, BaseType, "BaseType", --count);
	}
	else if (num == 24)
	{
		int count = 4;
		processAddChild(0, FieldDecMore, "FieldDecMore", --count);
		processAddChild(1, SEMI, "SEMI", --count);
		processAddChild(0, IdList, "IDList", --count);
		processAddChild(0, ArrayType, "ArrayType", --count);
	}
	else if (num == 25)
	{
		;
	}
	else if (num == 26)
	{
		int count = 1;
		processAddChild(0, FieldDecList, "FieldDecList", --count);
	}
	else if (num == 27)
	{
		int count = 2;
		processAddChild(0, IdMore, "IDMore", --count);
		processAddChild(1, ID, "ID", --count);
	}
	else if (num == 28)
	{
		;
	}
	else if (num == 29)
	{
		int count = 2;
		processAddChild(0, IdList, "IDList", --count);
		processAddChild(1, COMMA, "COMMA", --count);
	}
	else if (num == 30)
	{
		;
	}
	else if (num == 31)
	{
		int count = 1;
		processAddChild(0, VarDeclaration, "VarDeclaration", --count);
	}
	else if (num == 32)
	{
		int count = 2;
		processAddChild(0, VarDecList, "VarDecList", --count);
		processAddChild(1, VAR, "VAR", --count);
	}
	else if (num == 33)
	{
		int count = 4;
		processAddChild(0, VarDecMore, "VarDecMore", --count);
		processAddChild(1, SEMI, "SEMI", --count);
		processAddChild(0, VarIdList, "VarIDList", --count);
		processAddChild(0, TypeName, "TypeName", --count);
	}
	else if (num == 34)
	{
		;
	}
	else if (num == 35)
	{
		int count = 1;
		processAddChild(0, VarDecList, "VarDecList", --count);
	}
	else if (num == 36)
	{
		int count = 2;
		processAddChild(0, VarIdMore, "VarIDMore", --count);
		processAddChild(1, ID, "ID", --count);
	}
	else if (num == 37)
	{
		;
	}
	else if (num == 38)
	{
		int count = 2;
		processAddChild(0, VarIdList, "VarIDList", --count);
		processAddChild(1, COMMA, "COMMA", --count);
	}
	else if (num == 39)
	{
		;
	}
	else if (num == 40)
	{
		int count = 1;
		processAddChild(0, ProcDeclaration, "ProcDeclaration", --count);
	}
	else if (num == 41)
	{
		int count = 9;
		processAddChild(0, ProcDecMore, "ProcDecMore", --count);
		processAddChild(0, ProcBody, "ProcBody", --count);
		processAddChild(0, ProcDecPart, "ProcDecPart", --count);
		processAddChild(1, SEMI, "SEMI", --count);
		processAddChild(1, RPAREN, "RPAREN", --count);
		processAddChild(0, ParamList, "ParamList", --count);
		processAddChild(1, LPAREN, "LPAREN", --count);
		processAddChild(0, ProcName, "ProcName", --count);
		processAddChild(1, PROCEDURE, "PROCEDURE", --count);
	}
	else if (num == 42)
	{
		;
	}
	else if (num == 43)
	{
		int count = 1;
		processAddChild(0, ProcDeclaration, "ProcDeclaration", --count);
	}
	else if (num == 44)
	{
		int count = 1;
		processAddChild(1, ID, "ID", --count);
	}
	else if (num == 45)
	{
		;
	}
	else if (num == 46)
	{
		int count = 1;
		processAddChild(0, ParamDecList, "ParamDecList", --count);
	}
	else if (num == 47)
	{
		int count = 2;
		processAddChild(0, ParamMore, "ParamMore", --count);
		processAddChild(0, Param, "Param", --count);
	}
	else if (num == 48)
	{
		;
	}
	else if (num == 49)
	{
		int count = 2;
		processAddChild(0, ParamDecList, "ParamDecList", --count);
		processAddChild(1, SEMI, "SEMI", --count);
	}
	else if (num == 50)
	{
		int count = 2;
		processAddChild(0, FormList, "FormList", --count);
		processAddChild(0, TypeName, "TypeName", --count);
	}
	else if (num == 51)
	{
		int count = 3;
		processAddChild(0, FormList, "FormList", --count);
		processAddChild(0, TypeName, "TypeName", --count);
		processAddChild(1, VAR, "VAR", --count);
	}
	else if (num == 52)
	{
		int count = 2;
		processAddChild(0, FidMore, "FidMore", --count);
		processAddChild(1, ID, "ID", --count);
	}
	else if (num == 53)
	{
		;
	}
	else if (num == 54)
	{
		int count = 2;
		processAddChild(0, FormList, "FormList", --count);
		processAddChild(1, COMMA, "COMMA", --count);
	}
	else if (num == 55)
	{
		int count = 1;
		processAddChild(0, DeclarePart, "DeclarePart", --count);
	}
	else if (num == 56)
	{
		int count = 1;
		processAddChild(0, ProgramBody, "ProgramBody", --count);
	}
	else if (num == 57)
	{
		int count = 3;
		processAddChild(1, END1, "END1", --count);
		processAddChild(0, StmList, "StmList", --count);
		processAddChild(1, BEGIN, "BEGIN", --count);
	}
	else if (num == 58)
	{
		int count = 2;
		processAddChild(0, StmMore, "StmMore", --count);
		processAddChild(0, Stm, "Stm", --count);
	}
	else if (num == 59)
	{
		;
	}
	else if (num == 60)
	{
		int count = 2;
		processAddChild(0, StmList, "StmList", --count);
		processAddChild(1, SEMI, "SEMI", --count);
	}
	else if (num == 61)
	{
		int count = 1;
		processAddChild(0, ConditionalStm, "ConditionalStm", --count);
	}
	else if (num == 62)
	{
		int count = 1;
		processAddChild(0, LoopStm, "LoopStm", --count);
	}
	else if (num == 63)
	{
		int count = 1;
		processAddChild(0, InputStm, "InputStm", --count);
	}
	else if (num == 64)
	{
		int count = 1;
		processAddChild(0, OutputStm, "OutputStm", --count);
	}
	else if (num == 65)
	{
		int count = 1;
		processAddChild(0, ReturnStm, "ReturnStm", --count);
	}
	else if (num == 66)
	{
		int count = 2;
		processAddChild(0, AssCall, "AssCall", --count);
		processAddChild(1, ID, "ID", --count);
	}
	else if (num == 67)
	{
		int count = 1;
		processAddChild(0, AssignmentRest, "AssignmentRest", --count);
	}
	else if (num == 68)
	{
		int count = 1;
		processAddChild(0, CallStmRest, "CallStmRest", --count);
	}
	else if (num == 69)
	{
		int count = 3;
		processAddChild(0, Exp, "Exp", --count);
		processAddChild(1, ASSIGN, "ASSIGN", --count);
		processAddChild(0, VariMore, "VariMore", --count);
	}
	else if (num == 70)
	{
		int count = 7;
		processAddChild(1, FI, "FI", --count);
		processAddChild(0, StmList, "StmList", --count);
		processAddChild(1, ELSE, "ELSE", --count);
		processAddChild(0, StmList, "StmList", --count);
		processAddChild(1, THEN, "THEN", --count);
		processAddChild(0, RelExp, "RelExp", --count);
		processAddChild(1, IF, "IF", --count);
	}
	else if (num == 71)
	{
		int count = 5;
		//processAddChild(1, SEMI, "SEMI", --count);
		processAddChild(1, ENDWH, "ENDWH", --count);
		processAddChild(0, StmList, "StmList", --count);
		processAddChild(1, DO, "DO", --count);
		processAddChild(0, RelExp, "RelExp", --count);
		processAddChild(1, WHILE, "WHILE", --count);
	}
	else if (num == 72)
	{
		int count = 4;
		processAddChild(1, RPAREN, "RPAREN", --count);
		processAddChild(0, InVar, "InVar", --count);
		processAddChild(1, LPAREN, "LPAREN", --count);
		processAddChild(1, READ, "READ", --count);
	}
	else if (num == 73)
	{
		int count = 1;
		processAddChild(1, ID, "ID", --count);
	}
	else if (num == 74)
	{
		int count = 4;
		processAddChild(1, RPAREN, "RPAREN", --count);
		processAddChild(0, Exp, "Exp", --count);
		processAddChild(1, LPAREN, "LPAREN", --count);
		processAddChild(1, WRITE, "WRITE", --count);
	}
	else if (num == 75)
	{
		int count = 1;
		processAddChild(1, RETURN1, "RETURN1", --count);
	}
	else if (num == 76)
	{
		int count = 3;
		processAddChild(1, RPAREN, "RPAREN", --count);
		processAddChild(0, ActParamList, "ActParamList", --count);
		processAddChild(1, LPAREN, "LPAREN", --count);
	}
	else if (num == 77)
	{
		;
	}
	else if (num == 78)
	{
		int count = 2;
		processAddChild(0, ActParamMore, "ActParamMore", --count);
		processAddChild(0, Exp, "Exp", --count);
	}
	else if (num == 79)
	{
		;
	}
	else if (num == 80)
	{
		int count = 2;
		processAddChild(0, ActParamList, "ActParamList", --count);
		processAddChild(1, COMMA, "COMMA", --count);
	}
	else if (num == 81)
	{
		int count = 2;
		processAddChild(0, OtherRelE, "OtherRelE", --count);
		processAddChild(0, Exp, "Exp", --count);
	}
	else if (num == 82)
	{
		int count = 2;
		processAddChild(0, Exp, "Exp", --count);
		processAddChild(0, CmpOp, "CmpOp", --count);
	}
	else if (num == 83)
	{
		int count = 2;
		processAddChild(0, OtherTerm, "OtherTerm", --count);
		processAddChild(0, Term, "Term", --count);
	}
	else if (num == 84)
	{
		;
	}
	else if (num == 85)
	{
		int count = 2;
		processAddChild(0, Exp, "Exp", --count);
		processAddChild(0, AddOp, "AddOp", --count);
	}
	else if (num == 86)
	{
		int count = 2;
		processAddChild(0, OtherFactor, "OtherFactor", --count);
		processAddChild(0, Factor, "Factor", --count);
	}
	else if (num == 87)
	{
		;
	}
	else if (num == 88)
	{
		int count = 2;
		processAddChild(0, Term, "Term", --count);
		processAddChild(0, MultOp, "MultOp", --count);
	}
	else if (num == 89)
	{
		int count = 3;
		processAddChild(1, RPAREN, "RPAREN", --count);
		processAddChild(0, Exp, "Exp", --count);
		processAddChild(1, LPAREN, "LPAREN", --count);
	}
	else if (num == 90)
	{
		int count = 1;
		processAddChild(1, INTC, "INTC", --count);
	}
	else if (num == 91)
	{
		int count = 1;
		processAddChild(0, Variable, "Variable", --count);
	}
	else if (num == 92)
	{
		int count = 2;
		processAddChild(0, VariMore, "VariMore", --count);
		processAddChild(1, ID, "ID", --count);
	}
	else if (num == 93)
	{
		;
	}
	else if (num == 94)
	{
		int count = 3;
		processAddChild(1, RMIDPAREN, "RMIDPAREN", --count);
		processAddChild(0, Exp, "Exp", --count);
		processAddChild(1, LMIDPAREN, "LMIDPAREN", --count);
	}
	else if (num == 95)
	{
		int count = 2;
		processAddChild(0, FieldVar, "FieldVar", --count);
		processAddChild(1, DOT, "DOT", --count);
	}
	else if (num == 96)
	{
		int count = 2;
		processAddChild(0, FieldVarMore, "FieldVarMore", --count);
		processAddChild(1, ID, "ID", --count);
	}
	else if (num == 97)
	{
		;
	}
	else if (num == 98)
	{
		int count = 3;
		processAddChild(1, RMIDPAREN, "RMIDPAREN", --count);
		processAddChild(0, Exp, "Exp", --count);
		processAddChild(1, LMIDPAREN, "LMIDPAREN", --count);
	}
	else if (num == 99)
	{
		int count = 1;
		processAddChild(1, LT, "LT", --count);
	}
	else if (num == 100)
	{
		int count = 1;
		processAddChild(1, EQ, "EQ", --count);
	}
	else if (num == 101)
	{
		int count = 1;
		processAddChild(1, PLUS, "PLUS", --count);
	}
	else if (num == 102)
	{
		int count = 1;
		processAddChild(1, MINUS, "MINUS", --count);
	}
	else if (num == 103)
	{
		int count = 1;
		processAddChild(1, TIMES, "TIMES", --count);
	}
	else if (num == 104)
	{
		int count = 1;
		processAddChild(1, OVER, "OVER", --count);
	}
}

//LL1表的初始化
void InitLL1Table()
{
	int i, j;

	for (i = 0; i < 104; i++)
		for (j = 0; j < 104; j++)
			LL1Table[i][j] = 0;

	LL1Table[Program][PROGRAM] = 1;

	LL1Table[ProgramHead][PROGRAM] = 2;

	LL1Table[ProgramName][ID] = 3;

	LL1Table[DeclarePart][TYPE] = 4;
	LL1Table[DeclarePart][VAR] = 4;
	LL1Table[DeclarePart][PROCEDURE] = 4;
	LL1Table[DeclarePart][BEGIN] = 4;

	LL1Table[TypeDec][VAR] = 5;
	LL1Table[TypeDec][PROCEDURE] = 5;
	LL1Table[TypeDec][BEGIN] = 5;

	LL1Table[TypeDec][TYPE] = 6;

	LL1Table[TypeDeclaration][TYPE] = 7;

	LL1Table[TypeDecList][ID] = 8;

	LL1Table[TypeDecMore][VAR] = 9;
	LL1Table[TypeDecMore][PROCEDURE] = 9;
	LL1Table[TypeDecMore][BEGIN] = 9;


	LL1Table[TypeDecMore][ID] = 10;

	LL1Table[TypeId][ID] = 11;

	LL1Table[TypeName][INTEGER] = 12;
	LL1Table[TypeName][CHAR1] = 12;

	LL1Table[TypeName][ARRAY] = 13;
	LL1Table[TypeName][RECORD] = 13;

	LL1Table[TypeName][ID] = 14;

	LL1Table[BaseType][INTEGER] = 15;

	LL1Table[BaseType][CHAR1] = 16;

	LL1Table[StructureType][ARRAY] = 17;

	LL1Table[StructureType][RECORD] = 18;

	LL1Table[ArrayType][ARRAY] = 19;

	LL1Table[Low][INTC] = 20;

	LL1Table[Top][INTC] = 21;

	LL1Table[RecType][RECORD] = 22;

	LL1Table[FieldDecList][INTEGER] = 23;
	LL1Table[FieldDecList][CHAR1] = 23;

	LL1Table[FieldDecList][ARRAY] = 24;

	LL1Table[FieldDecMore][END1] = 25;

	LL1Table[FieldDecMore][INTEGER] = 26;
	LL1Table[FieldDecMore][CHAR1] = 26;
	LL1Table[FieldDecMore][ARRAY] = 26;

	LL1Table[IdList][ID] = 27;

	LL1Table[IdMore][SEMI] = 28;

	LL1Table[IdMore][COMMA] = 29;

	LL1Table[VarDec][PROCEDURE] = 30;
	LL1Table[VarDec][BEGIN] = 30;

	LL1Table[VarDec][VAR] = 31;

	LL1Table[VarDeclaration][VAR] = 32;

	LL1Table[VarDecList][INTEGER] = 33;
	LL1Table[VarDecList][CHAR1] = 33;
	LL1Table[VarDecList][ARRAY] = 33;
	LL1Table[VarDecList][RECORD] = 33;
	LL1Table[VarDecList][ID] = 33;

	LL1Table[VarDecMore][PROCEDURE] = 34;
	LL1Table[VarDecMore][BEGIN] = 34;


	LL1Table[VarDecMore][INTEGER] = 35;
	LL1Table[VarDecMore][CHAR1] = 35;
	LL1Table[VarDecMore][ARRAY] = 35;
	LL1Table[VarDecMore][RECORD] = 35;
	LL1Table[VarDecMore][ID] = 35;

	LL1Table[VarIdList][ID] = 36;

	LL1Table[VarIdMore][SEMI] = 37;

	LL1Table[VarIdMore][COMMA] = 38;

	LL1Table[ProcDec][BEGIN] = 39;

	LL1Table[ProcDec][PROCEDURE] = 40;

	LL1Table[ProcDeclaration][PROCEDURE] = 41;

	LL1Table[ProcDecMore][BEGIN] = 42;

	LL1Table[ProcDecMore][PROCEDURE] = 43;

	LL1Table[ProcName][ID] = 44;

	LL1Table[ParamList][RPAREN] = 45;

	LL1Table[ParamList][INTEGER] = 46;
	LL1Table[ParamList][CHAR1] = 46;
	LL1Table[ParamList][ARRAY] = 46;
	LL1Table[ParamList][RECORD] = 46;
	LL1Table[ParamList][ID] = 46;
	LL1Table[ParamList][VAR] = 46;

	LL1Table[ParamDecList][INTEGER] = 47;
	LL1Table[ParamDecList][CHAR1] = 47;
	LL1Table[ParamDecList][ARRAY] = 47;
	LL1Table[ParamDecList][RECORD] = 47;
	LL1Table[ParamDecList][ID] = 47;
	LL1Table[ParamDecList][VAR] = 47;

	LL1Table[ParamMore][RPAREN] = 48;

	LL1Table[ParamMore][SEMI] = 49;

	LL1Table[Param][INTEGER] = 50;
	LL1Table[Param][CHAR1] = 50;
	LL1Table[Param][ARRAY] = 50;
	LL1Table[Param][RECORD] = 50;
	LL1Table[Param][ID] = 50;

	LL1Table[Param][VAR] = 51;

	LL1Table[FormList][ID] = 52;

	LL1Table[FidMore][SEMI] = 53;
	LL1Table[FidMore][RPAREN] = 53;

	LL1Table[FidMore][COMMA] = 54;

	LL1Table[ProcDecPart][TYPE] = 55;
	LL1Table[ProcDecPart][VAR] = 55;
	LL1Table[ProcDecPart][PROCEDURE] = 55;
	LL1Table[ProcDecPart][BEGIN] = 55;

	LL1Table[ProcBody][BEGIN] = 56;

	LL1Table[ProgramBody][BEGIN] = 57;

	LL1Table[StmList][ID] = 58;
	LL1Table[StmList][IF] = 58;
	LL1Table[StmList][WHILE] = 58;
	LL1Table[StmList][RETURN1] = 58;
	LL1Table[StmList][READ] = 58;
	LL1Table[StmList][WRITE] = 58;

	LL1Table[StmMore][END1] = 59;
	LL1Table[StmMore][ENDWH] = 59;
	LL1Table[StmMore][ELSE] = 59;
	LL1Table[StmMore][FI] = 59;

	LL1Table[StmMore][SEMI] = 60;

	LL1Table[Stm][IF] = 61;

	LL1Table[Stm][WHILE] = 62;

	LL1Table[Stm][READ] = 63;

	LL1Table[Stm][WRITE] = 64;

	LL1Table[Stm][RETURN1] = 65;

	LL1Table[Stm][ID] = 66;

	LL1Table[AssCall][ASSIGN] = 67;
	LL1Table[AssCall][LMIDPAREN] = 67;
	LL1Table[AssCall][DOT] = 67;


	LL1Table[AssCall][LPAREN] = 68;

	LL1Table[AssignmentRest][ASSIGN] = 69;
	LL1Table[AssignmentRest][LMIDPAREN] = 69;
	LL1Table[AssignmentRest][DOT] = 69;

	LL1Table[ConditionalStm][IF] = 70;


	LL1Table[LoopStm][WHILE] = 71;

	LL1Table[InputStm][READ] = 72;

	LL1Table[InVar][ID] = 73;

	LL1Table[OutputStm][WRITE] = 74;

	LL1Table[ReturnStm][RETURN1] = 75;

	LL1Table[CallStmRest][LPAREN] = 76;

	LL1Table[ActParamList][RPAREN] = 77;

	LL1Table[ActParamList][ID] = 78;
	LL1Table[ActParamList][INTC] = 78;
	LL1Table[ActParamList][LPAREN] = 78;

	LL1Table[ActParamMore][RPAREN] = 79;

	LL1Table[ActParamMore][COMMA] = 80;

	LL1Table[RelExp][LPAREN] = 81;
	LL1Table[RelExp][INTC] = 81;
	LL1Table[RelExp][ID] = 81;

	LL1Table[OtherRelE][LT] = 82;
	LL1Table[OtherRelE][EQ] = 82;

	LL1Table[Exp][LPAREN] = 83;
	LL1Table[Exp][INTC] = 83;
	LL1Table[Exp][ID] = 83;

	LL1Table[OtherTerm][LT] = 84;
	LL1Table[OtherTerm][EQ] = 84;
	LL1Table[OtherTerm][THEN] = 84;
	LL1Table[OtherTerm][DO] = 84;
	LL1Table[OtherTerm][RPAREN] = 84;
	LL1Table[OtherTerm][END1] = 84;
	LL1Table[OtherTerm][SEMI] = 84;
	LL1Table[OtherTerm][COMMA] = 84;
	LL1Table[OtherTerm][ENDWH] = 84;
	LL1Table[OtherTerm][ELSE] = 84;
	LL1Table[OtherTerm][FI] = 84;
	LL1Table[OtherTerm][RMIDPAREN] = 84;


	LL1Table[OtherTerm][PLUS] = 85;
	LL1Table[OtherTerm][MINUS] = 85;

	LL1Table[Term][LPAREN] = 86;
	LL1Table[Term][INTC] = 86;
	LL1Table[Term][ID] = 86;

	LL1Table[OtherFactor][PLUS] = 87;
	LL1Table[OtherFactor][MINUS] = 87;
	LL1Table[OtherFactor][LT] = 87;
	LL1Table[OtherFactor][EQ] = 87;
	LL1Table[OtherFactor][THEN] = 87;
	LL1Table[OtherFactor][ELSE] = 87;
	LL1Table[OtherFactor][FI] = 87;
	LL1Table[OtherFactor][DO] = 87;
	LL1Table[OtherFactor][ENDWH] = 87;
	LL1Table[OtherFactor][RPAREN] = 87;
	LL1Table[OtherFactor][END1] = 87;
	LL1Table[OtherFactor][SEMI] = 87;
	LL1Table[OtherFactor][COMMA] = 87;
	LL1Table[OtherFactor][RMIDPAREN] = 87;

	LL1Table[OtherFactor][TIMES] = 88;
	LL1Table[OtherFactor][OVER] = 88;

	LL1Table[Factor][LPAREN] = 89;

	LL1Table[Factor][INTC] = 90;

	LL1Table[Factor][ID] = 91;

	LL1Table[Variable][ID] = 92;

	LL1Table[VariMore][ASSIGN] = 93;
	LL1Table[VariMore][TIMES] = 93;
	LL1Table[VariMore][OVER] = 93;
	LL1Table[VariMore][PLUS] = 93;
	LL1Table[VariMore][MINUS] = 93;
	LL1Table[VariMore][LT] = 93;
	LL1Table[VariMore][EQ] = 93;
	LL1Table[VariMore][THEN] = 93;
	LL1Table[VariMore][ELSE] = 93;
	LL1Table[VariMore][FI] = 93;
	LL1Table[VariMore][DO] = 93;
	LL1Table[VariMore][ENDWH] = 93;
	LL1Table[VariMore][RPAREN] = 93;
	LL1Table[VariMore][END1] = 93;
	LL1Table[VariMore][SEMI] = 93;
	LL1Table[VariMore][COMMA] = 93;
	LL1Table[VariMore][RMIDPAREN] = 93;

	LL1Table[VariMore][LMIDPAREN] = 94;

	LL1Table[VariMore][DOT] = 95;

	LL1Table[FieldVar][ID] = 96;

	LL1Table[FieldVarMore][ASSIGN] = 97;
	LL1Table[FieldVarMore][TIMES] = 97;
	LL1Table[FieldVarMore][OVER] = 97;
	LL1Table[FieldVarMore][PLUS] = 97;
	LL1Table[FieldVarMore][MINUS] = 97;
	LL1Table[FieldVarMore][LT] = 97;
	LL1Table[FieldVarMore][EQ] = 97;
	LL1Table[FieldVarMore][THEN] = 97;
	LL1Table[FieldVarMore][ELSE] = 97;
	LL1Table[FieldVarMore][FI] = 97;
	LL1Table[FieldVarMore][DO] = 97;
	LL1Table[FieldVarMore][ENDWH] = 97;
	LL1Table[FieldVarMore][RPAREN] = 97;
	LL1Table[FieldVarMore][END1] = 97;
	LL1Table[FieldVarMore][SEMI] = 97;
	LL1Table[FieldVarMore][COMMA] = 97;

	LL1Table[FieldVarMore][LMIDPAREN] = 98;

	LL1Table[CmpOp][LT] = 99;

	LL1Table[CmpOp][EQ] = 100;

	LL1Table[AddOp][PLUS] = 101;

	LL1Table[AddOp][MINUS] = 102;

	LL1Table[MultOp][TIMES] = 103;

	LL1Table[MultOp][OVER] = 104;

}

TreeNode* programLL1()
{
	INDEX = 0;
	// readToken 再次使用 重新赋值为0

	NonTerminal stacktopN;

	Terminal stacktopT;

	InitLL1Table(); // 初始化LL1分析表

	TreeNode* root = NULL; // 语法树的根

	anlsstack->push(0, Program); // 分析栈 压入初始结点Program
	anlsstack->top->p = new TreeNode("Program"); // 新建树节点
	root = anlsstack->top->p; // 指向根 便于最后返回整棵树

	readToken(); // 读出INDEX=0对应token

	while (!anlsstack->isEmpty) // 循环结束条件
	{
		if (anlsstack->top->ntflag == 1) // 终极符
		{
			stacktopT = anlsstack->top->t; // 记录一下终极符是什么
			if (stacktopT == currentToken->wd.tok) // 比较一下 如果正确
			{
				//cout << currentToken->wd.str << " ";
				anlsstack->top->p->name = currentToken->wd.str; // 把节点的名字用实际代码命名
				anlsstack->top->p->tk = currentToken; // 因为是终极符 所以已经可以把token给树节点了
				// 上面这步很重要

				anlsstack->pop(); // 弹出分析栈
				//cout << currentToken->wd.str << endl;
				readToken(); // 再往后读一个token
			}
			else
			{
				//终极符不匹配
				string temp = "Line ";
				temp += to_string(currentToken->line);
				temp += " ";
				temp += "\"";
				temp += currentToken->wd.str;
				temp += "\" ";
				temp += "& T:";
				temp += to_string(currentToken->wd.tok);
				temp += " NotMatch Error!\n";
				printErrorMsg(temp);
			}
		}
		else // 非终极符
		{
			stacktopN = anlsstack->top->n; // 存一下非终极符

			int num = LL1Table[stacktopN][currentToken->wd.tok]; // LL1

			currentTree = anlsstack->top->p; // 记录一下将要被弹出的树节点
			// 后面将要扩展它的子树


			anlsstack->pop(); // 弹出分析栈

			process(num);

			//readToken(); // 再往后读一个token
		}
	}
	if (currentToken->wd.tok != ENDFILE)
	{
		string temp;
		temp += "NotEndFile Error!\n";
		printErrorMsg(temp);
	}

	return  root;
}

/****************************************************************/
/****************************************************************/

// 语义分析 建立符号表 检查语义错误
// 基于LL1树
// 这部分代码可读性比较差 请理解一下 因为我真的没什么时间...
// 根据语法树建立符号表 基于前序遍历 其实完全可以不从语法树中读 但是我看要求上是这么写的索性就这么做了
// 错误检测的部分基本上是对每个错误都编写了单独的检测函数 程序效率不高 但是比较可靠


// 这几个函数是建立符号表用的 思路应该一看就能懂吧 具体的细节需要对照LL1树来看 代码比较笨重但是很可靠
void typeSaveTable(TreeNode * t, SymbolTable* st)
{
	if (t == NULL)
		return;
	//cout << t->name << endl;
	if (t->name == "TypeDecList")
	{
		//cout << t->childIndex;
		string name = t->child[0]->child[0]->name;
		Token* tk = t->child[0]->child[0]->tk;

		string tempstr = t->child[2]->child[0]->child[0]->name;

		string type;
		if (tempstr == "integer" || tempstr == "char")
			type = t->child[2]->child[0]->child[0]->tk->wd.str;
		else
			type = t->child[2]->child[0]->child[0]->child[0]->tk->wd.str;


		string kind = "typeKind";

		//cout << name;
		st->addRecord(name, kind, type, tk);
	}

	for (int i = 0; i < 10; i++)
	{
		typeSaveTable(t->child[i], st);
	}
}

void varSaveTable(TreeNode * t, SymbolTable* st)
{
	if (t == NULL)
		return;

	if (t->name == "VarDecList")
	{
		string type;
		if (t->child[0]->child[0]->name == "BaseType")
		{
			type = t->child[0]->child[0]->child[0]->tk->wd.str;
		}
		else
		{
			type = t->child[0]->child[0]->tk->wd.str;
		}

		TreeNode* p = t->child[1];
		while (p->name == "VarIDList")
		{
			//cout << t->childIndex;
			string name = p->child[0]->name;
			Token* tk = p->child[0]->tk;
			string kind = "varKind ";

			st->addRecord(name, kind, type, tk);

			if (p->child[1]->child[1] == NULL)
				break;
			else
				p = p->child[1]->child[1];
		}
	}

	for (int i = 0; i < 10; i++)
	{
		varSaveTable(t->child[i], st);
	}
}

void procSaveTable(TreeNode * t, SymbolTable* st, SymbolTable* nextst)
{
	if (t == NULL)
		return;

	if (t->name == "ProcDeclaration")
	{
		nextst = new SymbolTable();//新的表
		string name = t->child[1]->child[0]->name;
		Token* tk = t->child[1]->child[0]->tk;
		string kind = "procKind";
		string type = t->child[0]->tk->wd.str;

		st->addRecord(name, kind, type, tk, nextst);

		TreeNode* temp = t->child[3];
		if (temp->child[0] == NULL)
		{
			varSaveTable(t->child[6], nextst);
			return;
		}

		if (temp->child[0]->child[0]->child[0]->child[0]->name == "BaseType")
		{
			type = temp->child[0]->child[0]->child[0]->child[0]->child[0]->tk->wd.str;
			name = temp->child[0]->child[0]->child[1]->child[0]->name;
			tk = temp->child[0]->child[0]->child[1]->child[0]->tk;
			kind = "varKind ";
			nextst->addRecord(name, kind, type, tk);
			nextst->paramcount++;

			temp = temp->child[0]->child[1];//parammore
		}
		else
		{
			type = temp->child[0]->child[0]->child[0]->child[0]->tk->wd.str;
			name = temp->child[0]->child[0]->child[1]->child[0]->name;
			tk = temp->child[0]->child[0]->child[1]->child[0]->tk;
			kind = "varKind ";
			nextst->addRecord(name, kind, type, tk);
			nextst->paramcount++;

			temp = temp->child[0]->child[1];
		}

		while (temp->child[1])
		{
			temp = temp->child[1];//param dec list
			if (temp->child[0]->child[0]->child[0]->name == "BaseType")
			{
				type = temp->child[0]->child[0]->child[0]->child[0]->tk->wd.str;
				name = temp->child[0]->child[1]->child[0]->name;
				tk = temp->child[0]->child[1]->child[0]->tk;
				kind = "varKind ";
				nextst->addRecord(name, kind, type, tk);
				nextst->paramcount++;

				temp = temp->child[1];
			}
			else
			{
				type = temp->child[0]->child[0]->child[0]->tk->wd.str;
				name = temp->child[0]->child[1]->child[0]->name;
				tk = temp->child[0]->child[1]->child[0]->tk;
				kind = "varKind ";
				nextst->addRecord(name, kind, type, tk);
				nextst->paramcount++;

				temp = temp->child[1];
			}
		}

		varSaveTable(t->child[6], nextst);
	}

	for (int i = 0; i < 10; i++)
	{
		procSaveTable(t->child[i], st, nextst);
	}
}
//


string bstr[16];
int bindex = 0;
void checkParam(TreeNode * t)//检查参数类型和个数
{
	if (t == NULL)
		return;

	// 注释掉的这段是以前的旧代码 当时没有考虑到函数参数可以直接填常量
	/*
	if (t->name == "Variable")
	{
		bstr[bindex] = t->child[0]->name;
		//cout<< t->child[0]->name<<endl;
		bindex++;
	}
	*/

	if (t->name == "Factor")
	{
		if (t->child[0]->child[0] != NULL)
		{
			bstr[bindex] = t->child[0]->child[0]->name;
			bindex++;
		}
		else
		{
			if (t->child[0]->tk->wd.tok == INTC)
			{
				//cout << t->child[0]->tk->wd.tok << endl;
				bstr[bindex] = "integerT0712"; //这里偷了个懒 如果发现这个参数是int常量 就把bstr里的值赋成integerT0712
				//integerT0712 本身没有什么意义 就是一个不太容易与变量名重复的值罢了 
				bindex++;
			}

		}
	}
	for (int i = 0; i < 10; i++)
	{
		checkParam(t->child[i]);
	}
}

void checkProgramBody(TreeNode * t, SymbolTable * st)
{
	if (t == NULL)
		return;
	/*
	// 检查是否有未声明的
	// 这段代码在下面的checkAssignAndDeclaration重新实现
	// 旧的这段代码也能用但是现在重复了就注释掉好了
	if(t->tk != NULL)
		if (t->tk->wd.tok == ID)
		{
			int flag = 0;
			for (int i = 0; i < st->index; i++)
			{
				if (t->name == st->table[i]->name)
				{
					flag = 1;
				}
			}
			if (flag == 0)
			{
				string temp = "Line ";
				temp += to_string(t->tk->line);
				temp += " ";
				temp += "\"";
				temp += t->name;
				temp += "\" ";
				temp += "NoDeclaration Error!\n";
				printErrorMsg(temp);
			}
		}
	*/
	// 检查函数参数是否正确 包括类型和数量
	// t->child[0]
	if (t->child[0] != NULL && t->child[0]->tk != NULL)
		if (t->child[0]->tk->wd.tok == ID)
		{
			for (int i = 0; i < st->index; i++)
			{
				// 遍历过程中 如果定位到了函数 接下来就开始检查参数个数和类型
				if (t->child[0]->name == st->table[i]->name&&st->table[i]->kind == "procKind")
				{
					//cout << t->child[0]->name << " " << st->table[i]->next->paramcount << endl;
					//Token* pt = t->child[0]->tk;
					//int pi = t->child[0]->tk->index;

					string astr[16]; //astr 存的是正确情况下的参数类型列表 bstr是实际的参数类型列表
					int aindex = 0;

					// astr的初始化很简单 因为已经有符号表了所以记录一下就行
					for (int j = 0; j < st->table[i]->next->paramcount; j++)
					{
						astr[aindex] = (st->table[i]->next)->table[j]->type;
						//cout << astr[aindex] << endl;
						aindex++;
					}

					checkParam(t);

					//cout << aindex << "  " << bindex << endl;
					if (aindex != bindex) // 如果参数个数不相等 那就直接报错
					{
						string temp = "Line ";
						temp += to_string(t->child[0]->tk->line);
						temp += " ";
						temp += "\"";
						temp += t->child[0]->name;
						temp += "\" ";
						temp += "ParamNum Error!\n";
						printErrorMsg(temp);
					}
					else // 参数类型相等 看看类型是不是按顺序匹配上了
					{
						for (int i = 0; i < aindex; i++)
						{
							string tempa = astr[i];
							string tempb = "default";
							if (bstr[i] == "integerT0712")//如果是int常量 就直接把类型写成integer
							{
								tempb = "integer";
							}
							else // 不是常量
							{
								for (int j = 0; j < st->index; j++)
								{
									if (bstr[i] == st->table[j]->name)
									{
										tempb = st->table[j]->type;
										break;
									}
								}
							}
							if (tempa != tempb) //不匹配 报类型错误
							{
								string temp = "Line ";
								temp += to_string(t->child[0]->tk->line);
								temp += " ";
								temp += "\"";
								temp += t->child[0]->name;
								temp += "\" ";
								temp += "ParamType Error!\n";
								printErrorMsg(temp);
							}
						}
					}

					bindex = 0;

				}
			}
		}

	//检查过程名是不是过程标识符 其实完全可以跟上面合并在一起 但是我怕出错就单独写了 
	for (int i = 0; i < INDEX; i++)
	{
		if (tokenList[i].wd.tok == LPAREN)
		{
			string temp = tokenList[i - 1].wd.str;
			int flag = 0;
			for (int j = 0; j < st->index; j++)
			{
				if (temp == st->table[j]->name && st->table[j]->type == "procedure")
				{
					flag = 1;
				}
			}
			if (flag == 0 && temp != "read" && temp != "write")
			{
				string temp = "Line ";
				temp += to_string(tokenList[i - 1].line);
				temp += " ";
				temp += "\"";
				temp += tokenList[i - 1].wd.str;
				temp += "\" ";
				temp += "ProcNotFound Error!\n";
				printErrorMsg(temp);
			}
		}
	}

	for (int i = 0; i < 10; i++)
	{
		checkProgramBody(t->child[i], st);
	}
}

void checkAssignAndDeclaration(TreeNode * t)// 这个函数的功能比较复杂 首先是检查赋值号左边的变量类型 还顺便看是否有未声明标识符
{
	// 需要对着LL1的树一点一点看
	// 我就不写注释了太麻烦了
	TreeNode * p0 = t->child[2]->child[0];
	TreeNode * p1 = t->child[3];
	int i0 = p0->tk->index;
	int i1 = p1->tk->index;
	for (int i = i0; i <= i1; i++)
	{
		if (tokenList[i].wd.tok == ASSIGN)
		{
			string tempstr = tokenList[i - 1].wd.str;
			int flag = 0;
			for (int j = 0; j < smbltable->index; j++)
			{
				if (tempstr == smbltable->table[j]->name && smbltable->table[j]->kind == "varKind ")
				{
					flag = 1;
				}
				if (tempstr == "]")
					flag = 1;
			}
			if (flag == 0)
			{
				string temp = "Line ";
				temp += to_string(tokenList[i - 1].line);
				temp += " ";
				temp += "\"";
				temp += tempstr;
				temp += "\" ";
				temp += "NotVarKind Error!\n";
				printErrorMsg(temp);
			}
		}
		if (tokenList[i].wd.tok == ID)
		{
			string tempstr = tokenList[i].wd.str;
			int flag = 0;
			for (int j = 0; j < smbltable->index; j++)
			{
				if (tempstr == smbltable->table[j]->name && smbltable->table[j]->kind != "typeKind ")
				{
					flag = 1;
				}


			}
			if (flag == 0)
			{
				string temp = "Line ";
				temp += to_string(tokenList[i].line);
				temp += " ";
				temp += "\"";
				temp += tempstr;
				temp += "\" ";
				temp += "NoDeclaration Error!\n";
				printErrorMsg(temp);
			}
		}
	}

	int procedure[16];
	int begin[16];
	int end[16];
	int pindex = 0;
	for (int i = 0; i < i0; i++)
	{
		if (tokenList[i].wd.str == "procedure")
		{
			procedure[pindex] = tokenList[i].index;
			for (int j = i; j < i0; j++)
			{
				if (tokenList[j].wd.str == "begin")
				{
					begin[pindex] = tokenList[j].index;
					for (int k = j; k < i0; k++)
					{
						if (tokenList[k].wd.str == "end")
						{
							end[pindex] = tokenList[k].index;
							pindex++;
							break;
						}
					}
					break;
				}
			}
		}
	}

	for (int i = 0; i < pindex; i++)
	{
		//cout << procedure[i] << " "
			//<< begin[i] << " "
			//<< end[i] << " " << endl;
		string procname = tokenList[procedure[i] + 1].wd.str;
		//cout << procname << endl;
		SymbolTable* p = NULL;
		for (int j = 0; j < smbltable->index; j++)
		{
			if (smbltable->table[j]->name == procname)
			{
				// cout << smbltable->table[j]->name << endl;
				p = smbltable->table[j]->next;
			}
		}
		for (int j = begin[i]; j < end[i]; j++)
		{
			if (tokenList[j].wd.tok == ASSIGN)
			{
				string tempstr = tokenList[j - 1].wd.str;
				int flag = 0;
				for (int k = 0; k < p->index; k++)
				{
					if (tempstr == p->table[k]->name && p->table[k]->kind == "varKind ")
					{
						flag = 1;
					}
					if (tempstr == "]")
						flag = 1;
				}
				if (flag == 0)
				{
					string temp = "Line ";
					temp += to_string(tokenList[j - 1].line);
					temp += " ";
					temp += "\"";
					temp += tempstr;
					temp += "\" ";
					temp += "NotVarKind Error!\n";
					printErrorMsg(temp);
				}
			}
			if (tokenList[j].wd.tok == ID)
			{
				string tempstr = tokenList[j].wd.str;
				int flag = 0;
				for (int k = 0; k < p->index; k++)
				{
					if (tempstr == p->table[k]->name && p->table[k]->kind == "varKind ")
					{
						flag = 1;
					}
				}
				if (flag == 0)
				{
					string temp = "Line ";
					temp += to_string(tokenList[j].line);
					temp += " ";
					temp += "\"";
					temp += tempstr;
					temp += "\" ";
					temp += "NoDeclaration Error!\n";
					printErrorMsg(temp);
				}
			}
		}
	}

}

void checkAssignLeftRight(TreeNode * t)// 检查赋值符号左右的类型的
{
	TreeNode * p0 = t->child[2]->child[0];
	TreeNode * p1 = t->child[3];
	int i0 = p0->tk->index;
	int i1 = p1->tk->index;
	for (int i = i0; i <= i1; i++)
	{
		if (tokenList[i].wd.tok == ASSIGN)
		{
			string tempstr = tokenList[i - 1].wd.str;
			int flag = 0;
			for (int j = 0; j < smbltable->index; j++)
			{
				if (tempstr == smbltable->table[j]->name && smbltable->table[j]->type == "integer")
				{
					flag = 1;
					break;
				}
				if (tempstr == "]")
					flag = 1;
			}
			if (flag == 0)
			{
				string temp = "Line ";
				temp += to_string(tokenList[i].line);
				temp += " ";
				temp += "\"";
				temp += ":=";
				temp += "\" ";
				temp += "NotSameType Error!\n";
				printErrorMsg(temp);
			}

			for (int j = 0; j < smbltable->index; j++)
			{
				if (tempstr == smbltable->table[j]->name && smbltable->table[j]->type == "integer")
				{
					for (int k = i + 1; tokenList[k].wd.str != ";"; k++)
					{
						if (tokenList[k].wd.tok == LPAREN);
						else if (tokenList[k].wd.tok == RPAREN);
						else if (tokenList[k].wd.tok == INTC);
						else if (tokenList[k].wd.tok == PLUS);
						else if (tokenList[k].wd.tok == MINUS);
						else if (tokenList[k].wd.tok == TIMES);
						else if (tokenList[k].wd.tok == OVER);
						else if (tokenList[k].wd.tok == ID)
						{
							int flag = 0;
							for (int l = 0; l < smbltable->index; l++)
							{
								if (tokenList[k].wd.str == smbltable->table[l]->name && smbltable->table[l]->type == "integer")
									flag = 1;
								if (tempstr == "]")
									flag = 1;
							}
							if (flag == 0)
							{
								string temp = "Line ";
								temp += to_string(tokenList[i].line);
								temp += " ";
								temp += "\"";
								temp += ":=";
								temp += "\" ";
								temp += "NotSameType Error!\n";
								printErrorMsg(temp);
							}
						}

					}
				}
			}

		}
	}

	int procedure[16];
	int begin[16];
	int end[16];
	int pindex = 0;
	for (int i = 0; i < i0; i++)
	{
		if (tokenList[i].wd.str == "procedure")
		{
			procedure[pindex] = tokenList[i].index;
			for (int j = i; j < i0; j++)
			{
				if (tokenList[j].wd.str == "begin")
				{
					begin[pindex] = tokenList[j].index;
					for (int k = j; k < i0; k++)
					{
						if (tokenList[k].wd.str == "end")
						{
							end[pindex] = tokenList[k].index;
							pindex++;
							break;
						}
					}
					break;
				}
			}
		}
	}

	for (int i = 0; i < pindex; i++)
	{
		//cout << procedure[i] << " "
			//<< begin[i] << " "
			//<< end[i] << " " << endl;
		string procname = tokenList[procedure[i] + 1].wd.str;
		//cout << procname << endl;
		SymbolTable* p = NULL;
		for (int j = 0; j < smbltable->index; j++)
		{
			if (smbltable->table[j]->name == procname)
			{
				// cout << smbltable->table[j]->name << endl;
				p = smbltable->table[j]->next;
			}
		}

		for (int j = begin[i]; j < end[i]; j++)
		{

			if (tokenList[j].wd.tok == ASSIGN)
			{
				string tempstr = tokenList[j - 1].wd.str;
				int flag = 0;
				for (int k = 0; k < p->index; k++)
				{
					if (tempstr == p->table[k]->name && p->table[k]->type == "integer")
					{
						flag = 1;
						break;
					}
				}
				if (flag == 0)
				{
					string temp = "Line ";
					temp += to_string(tokenList[j].line);
					temp += " ";
					temp += "\"";
					temp += ":=";
					temp += "\" ";
					temp += "NotSameType Error!\n";
					printErrorMsg(temp);
				}
				for (int k = 0; k < p->index; k++)
				{
					if (tempstr == p->table[k]->name && p->table[k]->type == "integer")
					{
						for (int l = j + 1; tokenList[l].wd.str != ";"; l++)
						{
							if (tokenList[l].wd.tok == LPAREN);
							else if (tokenList[l].wd.tok == RPAREN);
							else if (tokenList[l].wd.tok == INTC);
							else if (tokenList[l].wd.tok == PLUS);
							else if (tokenList[l].wd.tok == MINUS);
							else if (tokenList[l].wd.tok == TIMES);
							else if (tokenList[l].wd.tok == OVER);
							else if (tokenList[l].wd.tok == ID)
							{
								int flag = 0;
								for (int m = 0; m < p->index; l++)
								{
									if (tokenList[l].wd.str == p->table[m]->name && p->table[m]->type == "integer")
										flag = 1;
								}
								if (flag == 0)
								{
									string temp = "Line ";
									temp += to_string(tokenList[j].line);
									temp += " ";
									temp += "\"";
									temp += ":=";
									temp += "\" ";
									temp += "NotSameType Error!\n";
									printErrorMsg(temp);
								}
							}

						}
					}
				}

			}


		}
	}
}

void checkRange()//检查array越界的
{
	string type[16];
	int min[16];
	int max[16];
	int iarr = 0;

	for (int i = 0; i < INDEX; i++)
	{
		if (tokenList[i].wd.tok == UNDERANGE)
		{
			min[iarr] = atoi(tokenList[i - 1].wd.str.c_str());
			max[iarr] = atoi(tokenList[i + 1].wd.str.c_str());
			type[iarr] = tokenList[i - 5].wd.str;
			//			cout << min[iarr] << endl;
			//			cout << max[iarr] << endl;
			iarr++;
		}
	}

	for (int i = 0; i < INDEX; i++)
	{
		if (tokenList[i].wd.str == "[" && tokenList[i + 1].wd.tok == INTC && tokenList[i + 2].wd.str == "]")
		{
			string temptype;
			int flag = 0;
			for (int j = 0; j < smbltable->index; j++)
			{
				if (smbltable->table[j]->name == tokenList[i - 1].wd.str && smbltable->table[j]->kind == "varKind ")
				{
					temptype = smbltable->table[j]->type;
					flag = 1;
					break;
				}
			}
			if (flag == 0)
			{
				string temp = "Line ";
				temp += to_string(tokenList[i].line);
				temp += " ";
				temp += "\"";
				temp += tokenList[i - 1].wd.str;
				temp += "\" ";
				temp += "NotVar Error!\n";
				printErrorMsg(temp);
			}
			//cout << tokenList[i + 1].wd.str << endl;
			for (int j = 0; j < iarr; j++)
			{

				if (type[j] == temptype)
				{
					if (atoi(tokenList[i + 1].wd.str.c_str()) <= max[j] && atoi(tokenList[i + 1].wd.str.c_str()) >= min[j]);
					else
					{
						string temp = "Line ";
						temp += to_string(tokenList[i + 1].line);
						temp += " ";
						temp += "\"";
						temp += tokenList[i + 1].wd.str;
						temp += "\" ";
						temp += "OutRange Error!\n";
						printErrorMsg(temp);
					}
				}
			}
		}
	}

}
/****************************************************************/
/****************************************************************/



//词法分析
void STEP1()
{
	for (int i = 0; i < 1024; i++)
	{
		tokenList[i].index = i;
	}
	FILE* fp;
	fp = fopen(SOURCE, "r");
	lexicalAnalyse(fp);
	printTokenList();
}
//递归下降语法分析
void STEP2a()
{
	TreeNode* t = program();
	printTree(t, 0, false, OUT2a);
}
//LL1语法分析
void STEP2b()
{
	treeroot = programLL1();
	printTreeLL1(treeroot, 0, false, OUT2b);
}
// 语义分析
void STEP3()
{
	TreeNode* t = treeroot;

	t = t->child[1];
	//cout << t->name;
	// 现在t是DeclarePart
	for (int i = 0; t->child[i] != NULL; i++)
	{
		TreeNode* temp = t->child[i];
		if (temp->name == "TypeDec")
			typeSaveTable(temp, smbltable);
		else if (temp->name == "VarDec")
			varSaveTable(temp, smbltable);
		else if (temp->name == "ProcDec")
		{
			SymbolTable * p = NULL;
			procSaveTable(temp, smbltable, p);
		}
	}

	TreeNode* t1 = treeroot;

	t1 = t1->child[2];
	checkProgramBody(t1, smbltable);

	checkAssignAndDeclaration(treeroot);

	checkAssignLeftRight(treeroot);

	checkRange();

	smbltable->printTable();
}
//
void STEP4()
{
	ofstream output(OUTerror);
	cout << "Compile Successful! No Errors Found!" << endl;;
	output << "Compile Successful! No Errors Found!\n";
}

int main()
{
	STEP1();

	printf("\n\n\n\n\n\n\n\n");

	STEP2a();

	printf("\n\n\n\n\n\n\n\n");

	STEP2b();

	printf("\n\n\n\n\n\n\n\n");

	STEP3();

	printf("\n\n\n\n\n\n\n\n");

	STEP4();
}

/*
program p

type
	sint = integer;

		srecord = record
				integer  x;
				char     y,z;
				  end;

	sarray = array [0..5] of integer;

var
	integer i1,i2;
	char c1;
	sarray sarr1;
	srecord sr1;
	integer i3;


procedure	q(integer i; srecord s);
var
	integer a;
	char c;
begin
	i := 1;
	a := 1;
	write(a);
end


procedure 	p(srecord s);
var
	integer a;
	{srecord s;}
begin
	a:=1;
end


procedure	r();
var
	integer a;
begin
	a:=1;
	a:=10;
end


begin
	{sarray[6] := 1;}

	read(i1);
	r();
	q(i3, sr1);

	{while i1<10 do
		i1:=1;
	endwh;}

	if i1<10
	then
		i1:=i1+10;
	else
		i1:=i1-10;
	fi;

	p(sr1);
end.
*/
