#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include "LexicalAnalyzer.h"
#include "SyntaxAnalyzer.h"
using namespace std;

int main()
{
	FILE * source;
	source = fopen("C:\\Users\\Yichuan\\Desktop\\input.txt", "r");
	LexicalAnalyzer la = LexicalAnalyzer();
	la.lexicalAnalyse(source);
	la.printTokenList();
	std::cout << "Lexical Analyse Succeeds!\n";

	token list[1000];
	
	for (int i = 0; i < 1000; i++)
	{
		list[i] = la.tokenList[i];
		//cout << list[i].wd.str;
	}
	
	//SyntaxAnalyzer sa = SyntaxAnalyzer(list);
	
	//sa.printTree(sa.parse());
}

