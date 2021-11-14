#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <map>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <stack>
using namespace std;
char str[256];
char token[20];
int sst = 0;   //表示句子读的位置 sentenceStart
int tst = 0;   //表示词读的位置 tokenStart
int symed = 0; //表示存储符号的词组的最后位置
int symst = 0; //表示 存储符号的词组的当前读取
char key[9][15] = {"int", "main", "return", "const"};
char keyOut[9][15] = {"Int", "Main", "Return", "Const"};
char funcCall[9][15] = {"getint", "putint", "getch", "putch"};
char funcCallOut[9][15] = {"Func(getint)", "Func(putint)", "Func(getch)", "Func(putch)"};
char sym[3005][20];
int ret = 0;		//程序出错的返回值
int tempRetNum = 0; //EXP()式子中的临时返回值
struct ExpElem
{
	int type; //1 Exp中的数字，2 运算符，3 寄存器，4 UnaryOp
	/* 
	1：数字的值
	2：1加法，2减法，3乘法，4除法，5取余
	3：寄存器的标号
	4：1正号 2负号
	*/
	int value;
};
struct ExpElem *tempExpStack;
stack<struct ExpElem> ExpStack; //用于生成计算值LLVM IR的栈
bool VarInInit = false;
bool LvalIsConst = false;
struct VarItem
{
	bool isConst;	 //是否是常量
	int registerNum; //寄存器的号码
};
map<string, struct VarItem>::iterator varIt; //Varmap变量迭代器
map<string, struct VarItem> VarMap;
int VarMapSt = 0; //当前新寄存器的值
struct FuncItem
{
	int RetType;		//函数返回类型 1为int 0为void
	vector<int> params; //函数参数列表
	string funcName;	//LLVM IR中的函数名
	int paramsNum;     //函数参数个数
};
map<string, struct FuncItem> FuncMap;
map<string, struct FuncItem>::iterator funcIt; //Funcmap变量迭代器
int LVal();
int getToken();
int CompUnit();
int FuncType();
int FuncDef();
int VarDef();
int Ident();
int Block();
int BlockItem();
int Stmt();
int Exp();
int AddExp();
int MulExp();
int UnaryExp();
int Decl();
int ConstDecl();
int Btype();
int ConstDef();
int ConstInitVal();
int ConstExp();
int VarDecl();
int InitVal();
int PrimaryExp();
void UnaryOp();
void Operation();
void OperationUnaryOp();
void FuncRParams();
void FuncCall();
FILE *fpin;
FILE *fpout;
int main(int argc, char *argv[])
{
	fpout = fopen("out.txt", "w");
	fpin = fopen("in.txt", "r");
	if (fpin == NULL)
	{
		printf("fpin error");
	}
	if (fpout == NULL)
	{
		printf("fpout error");
	}
	getToken();
	strcpy(token, sym[symst++]);
	ret = CompUnit();
	if (ret != 0)
		return ret;
	return 0;
}

//进制转换
void ChangeTen(int n, char str[])
{ //将n进制数转换成10进制数
	int len = strlen(str), i, sum = 0, t = 1;
	for (i = len - 1; i >= 0; i--)
	{
		if (str[i] >= 'A' && str[i] < 'G')
		{
			sum += (str[i] - 55) * t;
		}
		else if (str[i] >= 'a' && str[i] < 'g')
		{
			sum += (str[i] - 'a' + 10) * t;
		}
		else
		{
			sum += (str[i] - 48) * t;
		}
		t *= n;
	}
	sprintf(sym[symed++], "Number(%d)", sum);
}
//在表达式计算，Exp()类函数时 使用该函数报错
void error()
{
	ret = 120;
	printf("\nExp() error");
}
//词法分析
int getToken()
{
	int note = 0;
	tst = 0;
	while (fgets(str, 250, fpin) != NULL)
	{
		memset(token, 0, sizeof(token));
		int iskey = 0;
		sst = 0;
		while (sst < strlen(str))
		{
			memset(token, 0, sizeof(token));
			iskey = 0;
			char ch = str[sst];
			if (isspace(ch))
			{
				sst++;
			}
			else if (ch == '/' && str[sst + 1] == '/')
			{
				break;
			}
			else if (note == 1)
			{
				if (ch == '*' && str[sst + 1] == '/')
				{
					sst += 2;
					note = 0;
					continue;
				}
				sst++;
			}
			else if (ch == '/' && str[sst + 1] == '*')
			{
				note = 1;
				sst++;
			}
			//Ident或者关键字
			else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
			{
				token[tst++] = ch;
				ch = str[++sst];
				while ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
				{
					token[tst++] = ch;
					ch = str[++sst];
				}
				token[tst] = '\0';
				tst = 0;
				//判断是否是关键字
				for (int i = 0; i <= 3; i++)
				{
					if (strcmp(token, key[i]) == 0)
					{
						strcpy(sym[symed++], keyOut[i]);
						iskey = 1;
					}
				}
				//判断是否是调用函数
				for (int i = 0; i <= 3; i++)
				{
					if (strcmp(token, funcCall[i]) == 0)
					{
						strcpy(sym[symed++], funcCallOut[i]);
						iskey = 1;
					}
				}
				if (iskey != 1)
				{
					sprintf(sym[symed++], "Ident(%s)", token);
				}
			}
			//Number类
			else if (ch >= '0' && ch <= '9')
			{
				if (ch != '0')
				{
					token[tst++] = ch;
					ch = str[++sst];
					while (ch >= '0' && ch <= '9')
					{
						token[tst++] = ch;
						ch = str[++sst];
					}
					token[tst] = '\0';
					tst = 0;
					ChangeTen(10, token);
				}
				else
				{
					//16进制
					if (str[sst + 1] == 'x' || str[sst + 1] == 'X')
					{
						sst++;
						ch = str[++sst];
						if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
						{
							token[tst++] = ch;
							ch = str[++sst];
							while ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
							{
								token[tst++] = ch;
								ch = str[++sst];
							}
							token[tst] = '\0';
							tst = 0;
							ChangeTen(16, token);
						}
						//16进制Number出错
						else
						{
							return 16;
						}
					}
					//8进制
					else
					{
						if (str[sst + 1] >= '0' && str[sst + 1] <= '8')
						{
							ch = str[++sst];
							token[tst++] = ch;
							ch = str[++sst];
							while (ch >= '0' && ch <= '8')
							{
								token[tst++] = ch;
								ch = str[++sst];
							}
							token[tst] = '\0';
							tst = 0;
							ChangeTen(8, token);
						}
						else
						{
							char tempChange[2] = {"0"};
							ChangeTen(10, tempChange);
							sst++;
						}
					}
				}
			}
			else if (ch == '=')
			{
				if (str[++sst] == '=')
				{
					strcpy(sym[symed++], "Eq");
					sst++;
				}
				else
				{
					strcpy(sym[symed++], "Assign");
					sst++;
					sst--;
				}
			}
			else if (ch == ',')
			{
				strcpy(sym[symed++], "Comma");
				sst++;
			}
			else if (ch == ';')
			{
				strcpy(sym[symed++], "Semicolon");
				sst++;
			}
			else if (ch == '(')
			{
				strcpy(sym[symed++], "LPar");
				sst++;
			}
			else if (ch == ')')
			{
				strcpy(sym[symed++], "RPar");
				sst++;
			}
			else if (ch == '{')
			{
				strcpy(sym[symed++], "LBrace");
				sst++;
			}
			else if (ch == '}')
			{
				strcpy(sym[symed++], "RBrace");
				sst++;
			}
			else if (ch == '+')
			{
				strcpy(sym[symed++], "Plus");
				sst++;
			}
			else if (ch == '*')
			{
				strcpy(sym[symed++], "Mult");
				sst++;
			}
			else if (ch == '/')
			{
				strcpy(sym[symed++], "Div");
				sst++;
			}
			//			else if(ch=='<'){
			//				printf("Lt\n");sst++;
			//			}
			//			else if(ch=='>'){
			//				printf("Gt\n");sst++;
			//			}
			else if (ch == '-')
			{
				strcpy(sym[symed++], "Minus");
				sst++;
			}
			else if (ch == '%')
			{
				strcpy(sym[symed++], "Surplus");
				sst++;
			}
			else
			{
				printf("Err\n");
				return 0;
			}
			if (sst == strlen(str))
			{
				break;
			};
		}
	}
	return 0;
}
//初始化函数调用
void initFunc()
{
	//int getint();
	fprintf(fpout, "declare i32 @getint()\n");
	struct FuncItem f_getint;
	vector<int> Item_getint;
	f_getint.RetType = 1;
	f_getint.params = Item_getint;
	f_getint.funcName = "@getint";
	f_getint.paramsNum = 0;
	FuncMap["Func(getint)"] = f_getint;

	//int getch();
	fprintf(fpout, "declare i32 @getch()\n");
	struct FuncItem f_getch;
	vector<int> Item_getch;
	f_getch.RetType = 1;
	f_getch.params = Item_getch;
	f_getch.funcName = "@getch";
	f_getch.paramsNum = 0;
	FuncMap["Func(getch)"] = f_getch;

	//int getarray(int []);
	//fprintf(fpout,"declare i32 @getint()\n");

	//void putint(int);
	fprintf(fpout, "declare void @putint(i32)\n");
	struct FuncItem f_putint;
	vector<int> Item_putint;
	f_putint.RetType = 0;
	f_putint.params = Item_putint;
	f_putint.funcName = "@putint";
	f_putint.paramsNum = 1;
	FuncMap["Func(putint)"] = f_putint;
	//void putch(int);
	fprintf(fpout, "declare void @putch(i32)\n");
	struct FuncItem f_putch;
	vector<int> Item_putch;
	f_putch.RetType = 0;
	f_putch.params = Item_putch;
	f_putch.funcName = "@putch";
	f_putch.paramsNum = 1;
	FuncMap["Func(putch)"] = f_putch;

	//void putarray(int, int[]);
	//fprintf(fpout,"declare i32 @getint()\n");
}
//语法分析
int CompUnit()
{
	initFunc();
	fprintf(fpout, "define dso_local ");
	ret = FuncDef();
	if (ret != 0)
		return ret;
	strcpy(token, sym[symst++]);
	if (token[0] != 0)
	{
		printf("error in CompUnit");
	}
	return 0;
}
int Decl()
{
	if (strcmp(token, "Const") == 0)
	{
		strcpy(token, sym[symst++]);
		ret = ConstDecl();
		return ret;
	}
	else
	{
		return VarDecl();
	}
}
int ConstDecl()
{
	//const在Decl()中已经检测

	ret = Btype();
	if (ret != 0)
		return ret;
	strcpy(token, sym[symst++]);
	ret = ConstDef();
	strcpy(token, sym[symst++]);
	while (strcmp(token, "Comma") == 0)
	{
		strcpy(token, sym[symst++]);
		ret = ConstDef();
		if (ret != 0)
		{
			printf("error in ConstDecl");
			return ret;
		}
		strcpy(token, sym[symst++]);
	}
	if (strcmp(token, "Semicolon") != 0)
	{
		printf("error in ConstDecl ;");
		return 139;
	}
	return 0;
}
int Btype()
{
	if (strcmp(token, "Int") != 0)
	{
		printf("error in Btype()");
		return 143;
	}
	return 0;
}
int ConstDef()
{
	if (token[0] == 'I' && token[1] == 'd' && token[4] == 't' && token[5] == '(')
	{
		if ((VarMap.count((string)token)) == 1)
		{
			printf("error in ConstDef() Ident");
			return 135;
		}
		struct VarItem *tempVarItem = (struct VarItem *)malloc(sizeof(struct VarItem));
		tempVarItem->isConst = true;
		tempVarItem->registerNum = ++VarMapSt;
		VarMap[token] = *tempVarItem;

		fprintf(fpout, "    %%%d = alloca i32\n", VarMapSt);
		strcpy(token, sym[symst++]);
		if (strcmp(token, "Assign") != 0)
		{
			printf("error in ConstDef() '=' ");
			return 136;
		}
		strcpy(token, sym[symst++]);
		ret = ConstInitVal();
		if (ret != 0)
			return ret;
		tempExpStack = &ExpStack.top();
		if (tempExpStack->type == 1)
		{
			fprintf(fpout, "    store i32 %d, i32* %%%d\n", tempExpStack->value, tempVarItem->registerNum);
		}
		else if (tempExpStack->type == 3)
		{
			fprintf(fpout, "    store i32 %d, i32* %%%d\n", tempExpStack->value, tempVarItem->registerNum);
		}
		else
		{
			printf("error in VarDef()");
			error();
		}
		ExpStack.pop();
	}
	else
	{
		printf("error in ConstDef()");
		return 134;
	}
	return 0;
}
int ConstInitVal()
{
	return ConstExp();
}
int ConstExp()
{
	return AddExp();
}
int VarDecl()
{
	ret = Btype();
	if (ret != 0)
		return ret;
	strcpy(token, sym[symst++]);
	ret = VarDef();
	strcpy(token, sym[symst++]);
	while (strcmp(token, "Comma") == 0)
	{
		strcpy(token, sym[symst++]);
		ret = VarDef();
		if (ret != 0)
		{
			printf("error in VarDecl");
			return ret;
		}
		strcpy(token, sym[symst++]);
	}
	if (strcmp(token, "Semicolon") != 0)
	{
		printf("error in VarDecl ;");
		return 139;
	}
	return 0;
}
int VarDef()
{
	if (token[0] == 'I' && token[1] == 'd' && token[4] == 't' && token[5] == '(')
	{
		if (VarMap.count((string)token) == 1)
		{
			printf("error in VarDef() Ident");
			return 135;
		}
		varIt = VarMap.find((string)token);
		if (varIt != VarMap.end())
		{
			printf("error in varDef()");
			error();
		}
		struct VarItem *tempVarItem = (struct VarItem *)malloc(sizeof(struct VarItem));
		tempVarItem->isConst = false;
		tempVarItem->registerNum = ++VarMapSt;
		VarMap[token] = *tempVarItem;

		fprintf(fpout, "    %%%d = alloca i32\n", VarMapSt);
		if (strcmp(sym[symst], "Assign") == 0)
			strcpy(token, sym[symst++]);
		if (strcmp(token, "Assign") != 0)
		{
			return 0;
		}
		strcpy(token, sym[symst++]);
		ret = InitVal();
		if (ret != 0)
			return ret;
		tempExpStack = &ExpStack.top();
		if (tempExpStack->type == 1)
		{
			fprintf(fpout, "    store i32 %d, i32* %%%d\n", tempExpStack->value, tempVarItem->registerNum);
		}
		else if (tempExpStack->type == 3)
		{
			fprintf(fpout, "    store i32 %d, i32* %%%d\n", tempExpStack->value, tempVarItem->registerNum);
		}
		else
		{
			printf("error in VarDef()");
			error();
		}
		ExpStack.pop();
	}
	else
	{
		printf("error in VarDef()");
		return 134;
	}
	return 0;
}

int InitVal()
{
	int tempAns = Exp();
	return 0;
}
int FuncDef()
{
	ret = FuncType();
	if (ret != 0)
		return ret;
	strcpy(token, sym[symst++]);
	ret = Ident();
	if (ret != 0)
		return ret;
	strcpy(token, sym[symst++]);
	if (strcmp(token, "LPar") != 0)
	{
		printf("error in FuncDef '('");
		return 103;
	}
	fprintf(fpout, "(");
	strcpy(token, sym[symst++]);
	if (strcmp(token, "RPar") != 0)
	{
		printf("error in FuncDef ')'");
		return 104;
	}
	fprintf(fpout, ")");
	strcpy(token, sym[symst++]);
	ret = Block();
	if (ret != 0)
		return ret;
	return 0;
}
int FuncType()
{
	if (strcmp(token, "Int") != 0)
	{
		printf("error in FuncType");
		return 101;
	}
	fprintf(fpout, "i32 ");
	return 0;
}
int Ident()
{
	if (strcmp(token, "Main") == 0)
	{
		fprintf(fpout, "@main");
		return 0;
	}
	else if (token[0] == 'I' && token[1] == 'd' && token[2] == 'e' && token[5] == '(')
	{
	}
	else
	{
		printf("error in Ident");
		return 141;
	}
	return 0;
}
int Block()
{
	if (strcmp(token, "LBrace") != 0)
	{
		printf("error in Block '{'");
		return 105;
	}
	fprintf(fpout, "{\n");
	strcpy(token, sym[symst++]);
	while ((strcmp(token, "Const") == 0) || (strcmp(token, "Int") == 0) || \
	(strcmp(token, "Return") == 0) || (strcmp(token, "Semicolon") == 0) || \
	(token[0] == 'I' && token[1] == 'd' && token[2] == 'e' && token[5] == '(')||\
	(token[0]=='F'&&token[1]=='u'&&token[4]=='('))
	{
		ret = BlockItem();
		if (ret != 0)
			return ret;
		strcpy(token, sym[symst++]);
	}
	if (strcmp(token, "RBrace") != 0)
	{
		printf("error in Block '}'");
		return 107;
	}
	fprintf(fpout, "}");
	return 0;
}
int BlockItem()
{
	if (strcmp(token, "Const") == 0 || strcmp(token, "Int") == 0)
	{
		return Decl();
	}
	else
	{
		return Stmt();
	}
}
char tempNum[20];
int Stmt()
{
	if (strcmp(token, "Return") == 0)
	{
		strcpy(token, sym[symst++]);
		Exp();
		tempExpStack = &ExpStack.top();
		fprintf(fpout, "    ret");
		if (tempExpStack->type == 1)
		{
			fprintf(fpout, " i32 %d", tempExpStack->value);
		}
		else if (tempExpStack->type == 3)
		{
			fprintf(fpout, " i32 %%%d", tempExpStack->value);
		}
		else
		{
			printf("error in Stmt");
			error();
		}
		ExpStack.pop();
		strcpy(token, sym[symst++]);
		if (strcmp(token, "Semicolon") != 0)
		{
			printf("error in Stmt ';'");
			return 105;
		}
		fprintf(fpout, "\n");
		return 0;
	}
	else if (strcmp(token, "Semicolon") == 0)
	{
		return 0;
	}
	else if (token[0] == 'I' && token[1] == 'd' && token[2] == 'e' && token[5] == '(')
	{
		int retRegister = LVal();
		strcpy(token, sym[symst++]);
		if (strcmp(token, "Assign") != 0)
		{
			printf("error in Stmt '='");
			return 155;
		}

		strcpy(token, sym[symst++]);
		int tempAns = Exp();
		strcpy(token, sym[symst++]);
		if (tempExpStack->type == 1)
		{
			fprintf(fpout, "    store i32 %d i32* %%%d", tempExpStack->value, retRegister);
		}
		else if (tempExpStack->type == 3)
		{
			fprintf(fpout, "    store i32 %%%d i32* %%%d", tempExpStack->value, retRegister);
		}
		else
		{
			printf("error in Stmt");
			error();
		}
		if (strcmp(token, "Semicolon") != 0)
		{
			printf("error in Stmt ';'");
			return 166;
		}
		fprintf(fpout, "\n");
		return 0;
	}
	else
	{ //待修改
		Exp();
		//ExpStack.pop();
	}
	/*
	if(token[0]=='N'&&token[1]=='u'&&token[4]=='e'&&token[5]=='r'){
		sscanf(token,"%*[^(](%[^)]",tempNum);
		fprintf(fpout,"i32 %s",tempNum);
	}
	else{
		printf("error in Stmt 'Number'");
		return 106;
	}
	*/
	return 0;
}

int Exp()
{
	int ret = AddExp();
	if (ret != 0)
	{
		return ret;
	}
	return 0;
}
int AddExp()
{
	ret = MulExp();
	if (ret != 0)
		return ret;
	if (strcmp(sym[symst], "Plus") == 0 || strcmp(sym[symst], "Minus") == 0)
	{ //读后面一个词，判断是否正确
		strcpy(token, sym[symst++]);
	}
	while (strcmp(token, "Plus") == 0 || strcmp(token, "Minus") == 0)
	{
		char temptoken[20];
		strcpy(temptoken, token);
		strcpy(token, sym[symst++]);
		if (strcmp(temptoken, "Plus") == 0)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 2;
			tempExpStack->value = 1; //是加法
			ExpStack.push(*tempExpStack);
		}
		else
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 2;
			tempExpStack->value = 2; //是减法
			ExpStack.push(*tempExpStack);
		}
		ret = MulExp();
		if (ret != 0)
			return ret;
		Operation();
		if (strcmp(sym[symst], "Plus") == 0 || strcmp(sym[symst], "Minus") == 0)
		{ //读后面一个词，判断是否正确
			strcpy(token, sym[symst++]);
		}
	}
	if (ret != 0)
		return ret;
	return 0;
}
int MulExp()
{
	int RetNum = UnaryExp();
	if (ret != 0)
		return ret;
	if (strcmp(sym[symst], "Mult") == 0 || strcmp(sym[symst], "Div") == 0 || strcmp(sym[symst], "Surplus") == 0)
	{ //读后面一个词，判断是否正确
		strcpy(token, sym[symst++]);
	}
	while (strcmp(token, "Mult") == 0 || strcmp(token, "Div") == 0 || strcmp(token, "Surplus") == 0)
	{
		char temptoken[20];
		strcpy(temptoken, token);
		strcpy(token, sym[symst++]);
		if (strcmp(temptoken, "Mult") == 0)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 2;
			tempExpStack->value = 3; //是乘法
			ExpStack.push(*tempExpStack);
		}
		else if (strcmp(temptoken, "Div") == 0)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 2;
			tempExpStack->value = 4; //是除法
			ExpStack.push(*tempExpStack);
		}
		else
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 2;
			tempExpStack->value = 5; //是取余
			ExpStack.push(*tempExpStack);
		}
		RetNum = UnaryExp();
		if (ret != 0)
			return ret;
		Operation();
		if (strcmp(sym[symst], "Mult") == 0 || strcmp(sym[symst], "Div") == 0 || strcmp(sym[symst], "Surplus") == 0)
		{ //读后面一个词，判断是否正确
			strcpy(token, sym[symst++]);
		}
	}
	return 0;
}
int UnaryExp()
{
	//UnaryOp()
	if (strcmp(token, "Plus") == 0 || strcmp(token, "Minus") == 0)
	{
		UnaryOp();
		strcpy(token, sym[symst++]);
		UnaryExp();

		OperationUnaryOp();
	}
	else if (token[0] == 'F' && token[3] == 'c' && token[4] == '(')
	{
		FuncCall();
	}
	else
	{
		ret = PrimaryExp();
		if (ret != 0)
		{
			return ret;
		}
	}
	if (ret != 0)
	{
		return ret;
	}
	return 0;
}
void UnaryOp()
{
	if (strcmp(token, "Plus") == 0 || strcmp(token, "Minus") == 0)
	{
		if (strcmp(token, "Plus") == 0)
		{ //多余一个+-号，则以此来判断Number的正负
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 4;
			tempExpStack->value = 1; //是正号
			ExpStack.push(*tempExpStack);
		}
		else
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 4;
			tempExpStack->value = 2; //是负号
			ExpStack.push(*tempExpStack);
		}
	}
}
int PrimaryExp()
{
	if (strcmp(token, "LPar") == 0)
	{
		strcpy(token, sym[symst++]);
		Exp();
		strcpy(token, sym[symst++]);
		if (strcmp(token, "RPar") != 0)
		{
			printf("error in PrimaryExp() RPar");
			error();
		}
	}
	else if (token[0] == 'N' && token[1] == 'u' && token[4] == 'e' && token[5] == 'r')
	{
		//Number()
		int retPriNum;
		sscanf(token, "%*[^(](%[^)]", tempNum);
		sscanf(tempNum, "%d", &retPriNum);
		tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
		tempExpStack->type = 1;
		tempExpStack->value = retPriNum;
		ExpStack.push(*tempExpStack);
	}
	else if (token[0] == 'I' && token[1] == 'd' && token[2] == 'e' && token[5] == '(')
	{
		int LvalRegister = LVal();
		tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
		tempExpStack->type = 3;
		tempExpStack->value = ++VarMapSt;
		ExpStack.push(*tempExpStack);
		fprintf(fpout, "    %%%d = load i32, i32* %%%d\n", tempExpStack->value, LvalRegister);
	}
	else
	{
		printf("error in PrimaryExp()");
		error();
	}
	if (ret != 0)
	{
		return ret;
	}
	return 0;
}
int LVal()
{
	//检查是否是已经定义的变量
	bool declared = false;
	varIt = VarMap.find((string)token);
	if (varIt != VarMap.end())
	{
		declared = true;
	}

	if ((*varIt).second.isConst)
	{
		LvalIsConst = true;
	}
	else
	{
		VarInInit = true;
	}
	return (*varIt).second.registerNum; //返回寄存器数字
}
void FuncCall()
{
	funcIt = FuncMap.find((string)token);
	if (funcIt == FuncMap.end())
	{
		printf("error in FuncCall");
		throw "Error";
	}
	strcpy(token, sym[symst++]);
	if (strcmp(token, "LPar") != 0)
	{
		printf("error in FuncCall '('");
		throw "Error";
	}
	strcpy(token, sym[symst++]);
	if (strcmp(token, "LPar") != 0)
	{
		int paramsNum = (*funcIt).second.paramsNum;
		while (paramsNum > 0)
		{
			FuncRParams();
			paramsNum--;
			strcpy(token, sym[symst++]);
			if (paramsNum == 0)
				break;
			if (strcmp(token, "Comma") != 0)
			{
				printf("error in FuncCall FuncParams");
				throw "Error";
			}
			strcpy(token, sym[symst++]);
		}
	}

	if (strcmp(token, "RPar") != 0)
	{
		printf("error in FuncCall ')'");
		throw "Error";
	}
	if ((*funcIt).second.RetType == 0)
	{
		fprintf(fpout, "    call void %s", (*funcIt).second.funcName.c_str());
	}
	else if ((*funcIt).second.RetType == 1)
	{
		tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
		tempExpStack->type = 3;
		tempExpStack->value = ++VarMapSt;
		fprintf(fpout, "    %%%d = call i32 %s", tempExpStack->value, (*funcIt).second.funcName.c_str());
	}
	fprintf(fpout, "(");
	for (int i = 0; i < (*funcIt).second.paramsNum; i++)
	{
		if (i != 0)
		{
			fprintf(fpout, ",");
		}
		if (ExpStack.top().type == 1)
		{
			fprintf(fpout, "i32 %d", ExpStack.top().value);
		}
		else if (ExpStack.top().type == 3)
		{
			fprintf(fpout, "i32 %%%d", ExpStack.top().value);
		}
		else
		{
			printf("error in FuncCall");
			throw "Error";
		}
		ExpStack.pop();
	}
	if ((*funcIt).second.RetType)
	{
		ExpStack.push(*tempExpStack);
	}
	fprintf(fpout, ")\n");
}
void FuncRParams()
{
	ret = Exp();
	if (ret != 0)
	{
		printf("error in FuncRParams");
		throw "Error";
	}
}
void Operation()
{
	struct ExpElem num1, op, num2;
	num2 = ExpStack.top();
	if (num2.type != 1 && num2.type != 3)
	{
		printf("errro in Operation");
		error();
	}
	ExpStack.pop();
	op = ExpStack.top();
	if (op.type != 2)
	{
		printf("errro in Operation");
		error();
	}
	ExpStack.pop();
	num1 = ExpStack.top();
	if (num1.type != 1 && num1.type != 3)
	{
		printf("error in Operation");
		error();
	}
	ExpStack.pop();

	tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
	tempExpStack->type = 3;
	tempExpStack->value = ++VarMapSt;
	ExpStack.push(*tempExpStack);

	fprintf(fpout, "    %%%d = ", tempExpStack->value);
	switch (op.value)
	{
	case 1:
		fprintf(fpout, "add i32 ");
		break;
	case 2:
		fprintf(fpout, "sub i32 ");
		break;
	case 3:
		fprintf(fpout, "mul i32 ");
		break;
	case 4:
		fprintf(fpout, "sdiv i32 ");
		break;
	case 5:
		fprintf(fpout, "srem i32 ");
		break;
	}
	if (num1.type == 1)
	{
		fprintf(fpout, "%d,", num1.value);
	}
	else
	{
		fprintf(fpout, "%%%d,", num1.value);
	}
	if (num2.type == 1)
	{
		fprintf(fpout, " %d", num2.value);
	}
	else
	{
		fprintf(fpout, " %%%d", num2.value);
	}
	fprintf(fpout, "\n");
}
void OperationUnaryOp()
{
	struct ExpElem num = ExpStack.top();
	ExpStack.pop();
	if (num.type != 1 && num.type != 3)
	{
		printf("error in OperationUnaryOp");
		error();
	}

	if (ExpStack.empty())
	{
		ExpStack.push(num);
		return;
	}

	struct ExpElem op = ExpStack.top();
	ExpStack.pop();

	//已经到正常的加减法了
	if (op.type != 4)
	{
		ExpStack.push(op);
		ExpStack.push(num);
		return;
	}
	else if (op.value == 2)
	{
		if (num.type == 1)
		{
			fprintf(fpout, "    %%%d = sub i32 0, %d\n", ++VarMapSt, num.value);
		}
		else
		{
			fprintf(fpout, "    %%%d = sub i32 0, %%%d\n", ++VarMapSt, num.value);
		}
		num.type = 3;
		num.value = VarMapSt;
	}
	ExpStack.push(num);
}
