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
#include <list>
using namespace std;
char str[3024];
char token[2560];
int sst = 0;   //表示句子读的位置 sentenceStart
int tst = 0;   //表示词读的位置 tokenStart
int symed = 0; //表示存储符号的词组的最后位置
int symst = 0; //表示 存储符号的词组的当前读取
char key[12][15] = {"int", "main", "return", "const", "if", "else", "while", "break", "continue"};
char keyOut[12][15] = {"Int", "Main", "Return", "Const", "If", "Else", "While", "Break", "Continue"};
char funcCall[9][15] = {"getint", "putint", "getch", "putch"};
char funcCallOut[9][15] = {"Func(getint)", "Func(putint)", "Func(getch)", "Func(putch)"};
struct symType
{ //正在分析的词
	string name;
	/*类型：1~10 为关键字：int，main，return，const，if，else, while, break ,continue
		11~20为函数调用：getint putint getch putch
		21 Number 22 Ident
		51~ 符号 51 == 52 = 53 , 54 ; 55 ( 56 ) 57 { 58 } 59 + 60 *
				61 / 62 - 63 % 64 < 65 > 66 || 67 && 68 != 69 <= 70 >=
				71 ! 72 [ 73 ]
	*/
	int value; //如果是number类  会储存int型的值
	int type;
} sym[10050];
struct symType symNow;
int ret = 0;		//程序出错的返回值
int tempRetNum = 0; //EXP()式子中的临时返回值
struct ExpElem
{
	int type; //1 Exp中的数字，2 运算符，3 寄存器，4 UnaryOp, 5 比较符号
	/* 
	1：数字的值
	2：1加法，2减法，3乘法，4除法，5取余, 6 &&, 7 ||
	3：寄存器的标号
	4：1正号 2负号 3 Not
	5: 1 == 2 != 3 < 4 > 5<= 6>=
	*/
	int value;
	int value_1; //i1时的值，仅在type=3时使用
};
struct ExpElem *tempExpStack;
stack<struct ExpElem> ExpStack; //用于生成计算值LLVM IR的栈
bool VarInInit = false;
bool LvalIsConst = false;
struct VarItem
{
	bool isConst;	 //是否是常量
	int registerNum; //寄存器的号码
	int globalNum;	 //仅在是全局变量且是常量时使用
	int dimension;	 //如果是数组，是几维数组(在本编译器中都作为一维数组来使用)
	int d[10];		 // 如a[3][4]的数组，则d[0]=3 d[1]=4
	int arraySize;	 //对于数组 数组的大小
};
int Offset = 0;								 //多维数组初始化时的偏移量
int baseNum = 0;							 //多维数组偏移量计算中使用的，代表左花括号个数的值
bool arrayDef = false;						 //在init时，判断调用init函数的是不是array
int tempArr[1000];                           //在init全局数组时使用的临时数组。
map<string, struct VarItem>::iterator varIt; //Varmap变量迭代器
map<string, struct VarItem> GVarMap;		 //全局变量的Map
bool GlobalDef;								 //当值为true时，代表现在正在定义全局变量
bool IsGlobalVal;							 //当值为true时，说明当前load的变量是全局变量
map<string, struct VarItem> BVarMap;		 //代码块中的局部Map
list< map<string, struct VarItem> > VarMapList;
list< map<string, struct VarItem> >::reverse_iterator VarMapListIt; //VarMapList反向迭代器
int VarMapSt = 0;												  //当前新寄存器的值
int GVarMapst = 10000;											  //当前全局变量寄存器的值,和局部变量寄存器区分，从10000开始
struct VarItem *arrayDecl;										  //对于数组的初始化，需要保留数组定义时的信息以调用
// struct CondBlock{       //条件变量对应的代码块信息
// 	int registerNum; //寄存器的值
// 	int type;  //这是个什么类型呢 1 IF 2 Else 3 LOrd 4 LAnd 5 main
// 	int num;  //在这个类型是第几个
// 	bool wantB;  //想要上一个条件变量是什么布尔值
// };
// map<int, struct CondBlock> CondBlockMap;  //map[type]的num到多少了。
int condCount = 1; //该是第几个cond块了
bool condHasIcmp = false;
//以下的栈是用来fprintf跳转label的栈
stack<int> condCountFalseStack;
stack<int> condCountTrueStack;
stack<int> whileCountFalseStack;
stack<int> whileCountTrueStack;
stack<int> continueCountTrueStack;
map<bool, int> condCountMap; //没有用到，用bool值来判断条件变量块的编号

int mainCount = 1;		//准备从条件语句或者循环中，返回上一层，上一层的序号 m_{{maincount}}
int whileCondCount = 1; // 循环中的Cond的编号
struct FuncItem
{
	int RetType;		//函数返回类型 1为int 0为void
	vector<int> params; //函数参数列表
	string funcName;	//LLVM IR中的函数名
	int paramsNum;		//函数参数个数
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
int GlobalAddExp();
int GlobalMulExp();
int GlobalUnaryExp();
int GlobalPrimaryExp();
void UnaryOp();
void Operation();
void OperationUnaryOp();
void FuncRParams();
void FuncCall();
void Cond();
void LOrExp();
void LAndExp();
void EqExp();
void RelExp();
void OperationCond();
FILE *fpin;
FILE *fpout;
int main(int argc, char *argv[])
{
	fpout = fopen(argv[2], "w");
	fpin = fopen(argv[1], "r");
	if (fpin == NULL)
	{
		printf("fpin error");
	}
	if (fpout == NULL)
	{
		printf("fpout error");
	}
	try
	{
		getToken();
		symNow = sym[symst++];
		ret = CompUnit();
		if (ret != 0)
			return ret;
	}
	catch (...)
	{
		return 998;
	}
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
	sym[symed].value = sum;
	sym[symed].name = "Number";
	sym[symed++].type = 21;
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
	while (fgets(str, 3000, fpin) != NULL)
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
				while ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '\n')
				{
					if (ch == '\n')
					{
						fgets(str, 2000, fpin);
						sst = -1;
						ch = str[++sst];
					}
					else
					{
						token[tst++] = ch;
						ch = str[++sst];
					}
				}
				token[tst] = '\0';
				tst = 0;
				//判断是否是关键字
				for (int i = 0; i <= 8; i++)
				{
					if (strcmp(token, key[i]) == 0)
					{
						sym[symed].name = keyOut[i];
						sym[symed++].type = i + 1;
						iskey = 1;
					}
				}
				//判断是否是调用函数
				for (int i = 0; i <= 3; i++)
				{
					if (strcmp(token, funcCall[i]) == 0)
					{
						sym[symed].name = funcCallOut[i];
						sym[symed++].type = i + 11;
						iskey = 1;
					}
				}
				if (iskey != 1)
				{
					sym[symed].name = token;
					sym[symed++].type = 22;
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
					sym[symed].name = "Eq";
					sym[symed++].type = 51;
					sst++;
				}
				else
				{
					sym[symed].name = "Assign";
					sym[symed++].type = 52;
					sst++;
					sst--;
				}
			}
			else if (ch == ',')
			{
				sym[symed].name = "Comma";
				sym[symed++].type = 53;
				sst++;
			}
			else if (ch == ';')
			{
				sym[symed].name = "Semicolon";
				sym[symed++].type = 54;
				sst++;
			}
			else if (ch == '(')
			{
				sym[symed].name = "LPar";
				sym[symed++].type = 55;
				sst++;
			}
			else if (ch == ')')
			{
				sym[symed].name = "RPar";
				sym[symed++].type = 56;
				sst++;
			}
			else if (ch == '{')
			{
				sym[symed].name = "LBrace";
				sym[symed++].type = 57;
				sst++;
			}
			else if (ch == '}')
			{
				sym[symed].name = "RBrace";
				sym[symed++].type = 58;
				sst++;
			}
			else if (ch == '[')
			{
				sym[symed].name = "LBracket";
				sym[symed++].type = 72;
				sst++;
			}
			else if (ch == ']')
			{
				sym[symed].name = "RBracket";
				sym[symed++].type = 73;
				sst++;
			}
			else if (ch == '+')
			{
				sym[symed].name = "Plus";
				sym[symed++].type = 59;
				sst++;
			}
			else if (ch == '*')
			{
				sym[symed].name = "Mult";
				sym[symed++].type = 60;
				sst++;
			}
			else if (ch == '/')
			{
				sym[symed].name = "Div";
				sym[symed++].type = 61;
				sst++;
			}
			else if (ch == '-')
			{
				sym[symed].name = "Minus";
				sym[symed++].type = 62;
				sst++;
			}
			else if (ch == '%')
			{
				sym[symed].name = "Surplus";
				sym[symed++].type = 63;
				sst++;
			}
			else if (ch == '<')
			{
				if (str[++sst] == '=')
				{
					sym[symed].name = "ELt";
					sym[symed++].type = 69;
					sst++;
				}
				else
				{
					sym[symed].name = "Lt";
					sym[symed++].type = 64;
					sst++;
					sst--;
				}
			}
			else if (ch == '>')
			{
				if (str[++sst] == '=')
				{
					sym[symed].name = "EGt";
					sym[symed++].type = 70;
					sst++;
				}
				else
				{
					sym[symed].name = "Gt";
					sym[symed++].type = 65;
					sst++;
					sst--;
				}
			}
			else if (ch == '|')
			{
				if (str[++sst] == '|')
				{
					sym[symed].name = "Or";
					sym[symed++].type = 66;
					sst++;
				}
				else
				{
					printf("err in getToken() ||");
					throw "Error";
				}
			}
			else if (ch == '&')
			{
				if (str[++sst] == '&')
				{
					sym[symed].name = "And";
					sym[symed++].type = 67;
					sst++;
				}
				else
				{
					printf("err in getToken() ||");
					throw "Error";
				}
			}
			else if (ch == '!')
			{
				if (str[++sst] == '=')
				{
					sym[symed].name = "NEq";
					sym[symed++].type = 68;
					sst++;
				}
				else
				{
					sym[symed].name = "Not";
					sym[symed++].type = 71;
					sst--;
					sst++;
				}
			}
			else
			{
				printf("Err in getToken()\n");
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

	//void memset(pointer,0,size *sizeof(int))
	fprintf(fpout, "declare void @memset(i32*, i32, i32)\n");
}
//初始化条件变量的命名
// void initCond(){
// 	CondBlockMap[1].num = 1;
// 	CondBlockMap[2].num = 1;
// 	CondBlockMap[3].num = 1;
// 	CondBlockMap[4].num = 1;
// 	CondBlockMap[5].num = 1;
// }
//语法分析
int CompUnit()
{
	initFunc();
	//initCond();
	while ((symNow.type == 1 && (sym[symst + 1].type == 52 ||
								 sym[symst + 1].type == 54 || sym[symst + 1].type == 53 ||
								 sym[symst + 1].type == 72)) ||
		   symNow.type == 4)
	{
		GlobalDef = true;
		Decl();
		symNow = sym[symst++];
	}
	fprintf(fpout, "define dso_local ");
	GlobalDef = false;
	ret = FuncDef();
	if (ret != 0)
		throw "Error";
	symNow = sym[symst++];
	if (symst != symed + 1)
	{
		printf("error in CompUnit");
		throw "Error";
	}
	return 0;
}
int Decl()
{
	if (symNow.type == 4)
	{
		symNow = sym[symst++];
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
	symNow = sym[symst++];
	ret = ConstDef();
	symNow = sym[symst++];
	while (symNow.type == 53)
	{
		symNow = sym[symst++];
		ret = ConstDef();
		if (ret != 0)
		{
			printf("error in ConstDecl");
			return ret;
		}
		symNow = sym[symst++];
	}
	if (symNow.type != 54)
	{
		printf("error in ConstDecl ;");
		throw "Error";
	}
	return 0;
}
int Btype()
{
	if (symNow.type != 1)
	{
		printf("error in Btype()");
		throw "Error";
	}
	return 0;
}
int ConstDef()
{
	if (symNow.type == 22 && GlobalDef == false)
	{
		if ((BVarMap.count(symNow.name)) == 1)
		{
			printf("error in ConstDef() Ident");
			throw "Error";
		}
		struct VarItem *tempVarItem = (struct VarItem *)malloc(sizeof(struct VarItem));
		tempVarItem->isConst = true;
		tempVarItem->registerNum = ++VarMapSt;
		tempVarItem->dimension = 0;
		string tempName = symNow.name;
		int tempDimesion = 0;
		if (sym[symst].type == 72)
		{
			symNow = sym[symst++];
		}
		while (symNow.type == 72)
		{
			symNow = sym[symst++];
			tempVarItem->d[tempDimesion] = GlobalAddExp();
			tempDimesion++;
			symNow = sym[symst++];
			if (symNow.type != 73)
			{
				printf("error in ConstDef 数组 ]");
				throw "Error";
			}
			if (sym[symst].type == 72)
			{
				symNow = sym[symst++];
			}
		}
		if (tempDimesion != 0) //是数组
		{
			tempVarItem->dimension = tempDimesion;
			int arraySize = 1;
			for (int i = 0; i < tempDimesion; i++)
			{
				arraySize *= tempVarItem->d[i];
			}
			tempVarItem->arraySize = arraySize;
			BVarMap[tempName] = *tempVarItem;
			arrayDecl = tempVarItem; //把当前的数组变量信息留到InitVal用
			if (tempDimesion == 0)
				fprintf(fpout, "    %%x%d = alloca i32\n", VarMapSt);
			else
			{
				int tempVarMapSt = VarMapSt;
				fprintf(fpout, "    %%x%d = alloca [%d x i32]\n", VarMapSt++, arraySize);
				fprintf(fpout, "    %%x%d = getelementptr [%d x i32], [%d x i32]* %%x%d, i32 0, i32 0\n", VarMapSt, arraySize, arraySize, tempVarMapSt);
				fprintf(fpout, "    call void @memset(i32*  %%x%d,i32 0,i32 %d)\n", VarMapSt, arraySize);
			}

			if (sym[symst].type == 52)
				symNow = sym[symst++];
			if (symNow.type != 52)
			{
				return 0;
			}
			baseNum = 0;
			Offset = 0;
			symNow = sym[symst++];
			arrayDef = true;
			ret = ConstInitVal();
			// if (ret != 0)
			// 	return ret;
		}
		else{            //不是数组
			BVarMap[symNow.name] = *tempVarItem;
			fprintf(fpout, "    %%x%d = alloca i32\n", VarMapSt);
			symNow = sym[symst++];
			if (symNow.type != 52)
			{
				printf("error in ConstDef() '=' ");
				throw "Error";
			}
			symNow = sym[symst++];
			ret = ConstInitVal();
			// if (ret != 0)
			// 	return ret;
			tempExpStack = &ExpStack.top();
			if (tempVarItem->registerNum < 9999)
			{
				if (tempExpStack->type == 1)
				{
					fprintf(fpout, "    store i32 %d, i32* %%x%d\n", tempExpStack->value, tempVarItem->registerNum);
				}
				else if (tempExpStack->type == 3)
				{
					fprintf(fpout, "    store i32 %%x%d, i32* %%x%d\n", tempExpStack->value, tempVarItem->registerNum);
				}
				else
				{
					printf("error in ConstDef()");
					throw "Error";
					return ret;
				}
			}
			else if (tempVarItem->registerNum > 9999)
			{
				if (tempExpStack->type == 1)
				{
					fprintf(fpout, "    store i32 %d, i32* @x%d\n", tempExpStack->value, tempVarItem->registerNum);
				}
				else if (tempExpStack->type == 3)
				{
					fprintf(fpout, "    store i32 %%x%d, i32* @x%d\n", tempExpStack->value, tempVarItem->registerNum);
				}
				else
				{
					printf("error in ConstDef()");
					throw "Error";
					return ret;
				}
			}
			ExpStack.pop();
		}
	}
	else if (symNow.type == 22 && GlobalDef == true)
	{
		if ((GVarMap.count(symNow.name)) == 1)
		{
			printf("error in ConstDef() GVarMap");
			throw "Error";
		}
		struct VarItem *tempVarItem = (struct VarItem *)malloc(sizeof(struct VarItem));
		tempVarItem->isConst = true;
		tempVarItem->registerNum = ++GVarMapst;
		tempVarItem->globalNum = 0;
		tempVarItem->dimension = 0;
		string tempName = symNow.name;
		int tempDimesion = 0;
		if (sym[symst].type == 72)
		{
			symNow = sym[symst++];
		}
		while (symNow.type == 72)
		{
			symNow = sym[symst++];
			tempVarItem->d[tempDimesion] = GlobalAddExp();
			tempDimesion++;
			symNow = sym[symst++];
			if (symNow.type != 73)
			{
				printf("error in ConstDef 数组 ]");
				throw "Error";
			}
			if (sym[symst].type == 72)
			{
				symNow = sym[symst++];
			}
		}
		if (tempDimesion != 0) //是数组
		{
			tempVarItem->dimension = tempDimesion;
			int arraySize = 1;
			for (int i = 0; i < tempDimesion; i++)
			{
				arraySize *= tempVarItem->d[i];
			}
			tempVarItem->arraySize = arraySize;
			GVarMap[tempName] = *tempVarItem;
			arrayDecl = tempVarItem; //把当前的数组变量信息留到InitVal用
			if (sym[symst].type == 54 || sym[symst].type == 53)
			{
				fprintf(fpout, "@x%d = dso_local global [%d x i32] zeroinitializer\n", GVarMapst, arraySize);
				return 0;
			}
			symNow = sym[symst++];
			if (symNow.type != 52)
			{
				printf("error in VarDef() Global array '=' ");
				throw "Error";
			}
			baseNum = 0;
			Offset = 0;
			symNow = sym[symst++];
			arrayDef = true;
			memset(tempArr, 0, 1000*sizeof(int));
			if(arraySize>=950){
				printf("tempArr数组开小了");
			}
			ret = ConstInitVal();
			fprintf(fpout, "@x%d = dso_local global [%d x i32] [",GVarMapst, arraySize);
			for(int i=0;i<arraySize-1;i++){
				fprintf(fpout, "i32 %d, ", tempArr[i]);
			}
			fprintf(fpout,"i32 %d]\n",tempArr[arraySize-1]);
		}
		else{
			if(sym[symst].type == 54 || sym[symst].type == 53){     //没有初始化
			GVarMap[tempName] = *tempVarItem;
			fprintf(fpout,"@x%d = dso_local global i32 %d\n",GVarMapst,tempVarItem->globalNum);
			return 0;
			}
			symNow = sym[symst++];
			if (symNow.type != 52)
			{
				printf("error in ConstDef() Global '=' ");
				throw "Error";
			}
			symNow = sym[symst++];
			int resultNum = ConstInitVal();
			tempVarItem->globalNum = resultNum;
			GVarMap[tempName] = *tempVarItem;
			fprintf(fpout,"@x%d = dso_local global i32 %d\n",GVarMapst,resultNum);
		}
	}
	else
	{
		printf("error in ConstDef()");
		throw "Error";
	}
	return 0;
}
int ConstInitVal()
{
	if (GlobalDef == false)
	{
		if (arrayDef)
		{
			if (symNow.type == 57)
			{ //花括号  数组初始化
				symNow = sym[symst++];
				baseNum++;
				if (symNow.type == 58)
				{ //xxx = {}
					return 0;
				}
				else
				{
					ConstInitVal();
					if (sym[symst].type == 53)
					{
						symNow = sym[symst++];
					}
					int savedIndex = Offset;
					while (symNow.type == 53)
					{ //同一个花括号内，逗号右移一位
						symNow = sym[symst++];
						int temp = 1;
						for (int i = arrayDecl->dimension - 1; i >= baseNum; i--)
						{
							temp *= arrayDecl->d[i];
						}
						Offset += temp;
						InitVal();
						if (sym[symst].type == 53)
						{
							symNow = sym[symst++];
						}
					}
					symNow = sym[symst++];
					if (symNow.type != 58)
					{
						printf("error in InitVal 数组 }");
						throw "Error";
					}
					Offset = savedIndex;
					baseNum--;
				}
			}
			else
			{ //ConstInitVal -> ConstExp
				fprintf(fpout, "    %%x%d = getelementptr [%d x i32], [%d x i32]* %%x%d, i32 0, i32 %d\n", ++VarMapSt, arrayDecl->arraySize, arrayDecl->arraySize, arrayDecl->registerNum, Offset);
				int tempVarSt = VarMapSt;
				VarInInit = false;
				ConstExp();
				if (VarInInit) //常量初始化不能用变量
				{
					throw "Error";
				}
				tempExpStack = &ExpStack.top();
				if (arrayDecl->registerNum < 9999)
				{
					if (tempExpStack->type == 1)
					{
						fprintf(fpout, "    store i32 %d, i32* %%x%d\n", tempExpStack->value, tempVarSt);
					}
					else if (tempExpStack->type == 3)
					{
						fprintf(fpout, "    store i32 %%x%d, i32* %%x%d\n", tempExpStack->value, tempVarSt);
					}
					else
					{
						printf("error in InitVal 数组");
						throw "Error";
						return ret;
					}
				}
				ExpStack.pop();
				return 0;
			}
		}
		else
		{ //ConstInitVal -> ConstExp
			VarInInit = false;
			ConstExp();
			if (VarInInit) //常量初始化不能用变量
			{
				throw "Error";
			}
		}
	}
	else   //GLOBAL
	{
		if (arrayDef)
		{
			if (symNow.type == 57)
			{
				baseNum++;
				symNow = sym[symst++];
				if (symNow.type == 58)
				{ //xxx = {}
					return 0;
				}
				else
				{
					InitVal();
					if (sym[symst].type == 53)
					{
						symNow = sym[symst++];
					}
					int savedIndex = Offset;
					while (symNow.type == 53)
					{ //同一个花括号内，逗号右移一位
						symNow = sym[symst++];
						int temp = 1;
						for (int i = arrayDecl->dimension - 1; i >= baseNum; i--)
						{
							temp *= arrayDecl->d[i];
						}
						Offset += temp;
						InitVal();
						if (sym[symst].type == 53)
						{
							symNow = sym[symst++];
						}
					}
					symNow = sym[symst++];
					if (symNow.type != 58)
					{
						printf("error in InitVal Global数组 }");
						throw "Error";
					}
					Offset = savedIndex;
					baseNum--;
				}
			}
			else{
				VarInInit = false;
				int tempVarSt = VarMapSt;
				int tempAns = GlobalAddExp();
				if (VarInInit)
				{ //全局变量初始化不能用变量
					printf("全局变量初始化不能用变量");
					throw "Error";
				}
				tempArr[Offset] = tempAns;
				return 0;
			}
		}
		else
		{
			VarInInit = false;
			ConstExp();
			if (VarInInit)
			{ //全局变量初始化不能用变量
				throw "Error";
			}
		}
	}
	return 0;
}
int ConstExp()
{
	if (GlobalDef == false)
		return AddExp();
	else
	{
		int result = GlobalAddExp();
		return result;
	}
}
int VarDecl()
{
	ret = Btype();
	// if (ret != 0)
	// 	return ret;
	symNow = sym[symst++];
	ret = VarDef();
	symNow = sym[symst++];
	while (symNow.type == 53)
	{
		symNow = sym[symst++];
		ret = VarDef();
		if (ret != 0)
		{
			printf("error in VarDecl");
			return ret;
		}
		symNow = sym[symst++];
	}
	if (symNow.type != 54)
	{
		printf("error in VarDecl ;");
		throw "Error";
	}
	return 0;
}
int VarDef()
{
	if (symNow.type == 22 && GlobalDef == false)
	{
		if (BVarMap.count(symNow.name) == 1)
		{
			printf("error in VarDef() Ident");
			throw "Error";
		}
		varIt = BVarMap.find(symNow.name);
		if (varIt != BVarMap.end())
		{
			printf("error in varDef()");
			throw "Error";
			return ret;
		}
		struct VarItem *tempVarItem = (struct VarItem *)malloc(sizeof(struct VarItem));
		tempVarItem->isConst = false;
		tempVarItem->registerNum = ++VarMapSt;
		tempVarItem->dimension = 0;
		string tempName = symNow.name;

		int tempDimesion = 0;
		if (sym[symst].type == 72)
		{
			symNow = sym[symst++];
		}
		while (symNow.type == 72)
		{
			symNow = sym[symst++];
			tempVarItem->d[tempDimesion] = GlobalAddExp();
			tempDimesion++;
			symNow = sym[symst++];
			if (symNow.type != 73)
			{
				printf("error in ConstDef 数组 ]");
				throw "Error";
			}
			if (sym[symst].type == 72)
			{
				symNow = sym[symst++];
			}
		}

		if (tempDimesion != 0) //是数组
		{
			tempVarItem->dimension = tempDimesion;
			int arraySize = 1;
			for (int i = 0; i < tempDimesion; i++)
			{
				arraySize *= tempVarItem->d[i];
			}
			tempVarItem->arraySize = arraySize;
			BVarMap[tempName] = *tempVarItem;
			arrayDecl = tempVarItem; //把当前的数组变量信息留到InitVal用
			if (tempDimesion == 0)
				fprintf(fpout, "    %%x%d = alloca i32\n", VarMapSt);
			else
			{
				int tempVarMapSt = VarMapSt;
				fprintf(fpout, "    %%x%d = alloca [%d x i32]\n", VarMapSt++, arraySize);
				fprintf(fpout, "    %%x%d = getelementptr [%d x i32], [%d x i32]* %%x%d, i32 0, i32 0\n", VarMapSt, arraySize, arraySize, tempVarMapSt);
				fprintf(fpout, "    call void @memset(i32*  %%x%d,i32 0,i32 %d)\n", VarMapSt, arraySize);
			}

			if (sym[symst].type == 52)
				symNow = sym[symst++];
			if (symNow.type != 52)
			{
				return 0;
			}
			baseNum = 0;
			Offset = 0;
			symNow = sym[symst++];
			arrayDef = true;
			ret = InitVal();
			// if (ret != 0)
			// 	return ret;
		}
		else
		{ //不是数组
			BVarMap[symNow.name] = *tempVarItem;

			fprintf(fpout, "    %%x%d = alloca i32\n", VarMapSt);
			if (sym[symst].type == 52)
				symNow = sym[symst++];
			if (symNow.type != 52)
			{
				return 0;
			}
			symNow = sym[symst++];
			arrayDef = false;
			ret = InitVal();
			if (ret != 0)
				return ret;
			tempExpStack = &ExpStack.top();
			if (tempVarItem->registerNum < 9999)
			{
				if (tempExpStack->type == 1)
				{
					fprintf(fpout, "    store i32 %d, i32* %%x%d\n", tempExpStack->value, tempVarItem->registerNum);
				}
				else if (tempExpStack->type == 3)
				{
					fprintf(fpout, "    store i32 %%x%d, i32* %%x%d\n", tempExpStack->value, tempVarItem->registerNum);
				}
				else
				{
					printf("error in VarDef()");
					throw "Error";
					return ret;
				}
			}
			else if (tempVarItem->registerNum > 9999)
			{
				if (tempExpStack->type == 1)
				{
					fprintf(fpout, "    store i32 %d, i32* @x%d\n", tempExpStack->value, tempVarItem->registerNum);
				}
				else if (tempExpStack->type == 3)
				{
					fprintf(fpout, "    store i32 %%x%d, i32* @x%d\n", tempExpStack->value, tempVarItem->registerNum);
				}
				else
				{
					printf("error in VarDef()");
					throw "Error";
					return ret;
				}
			}
			ExpStack.pop();
		}
	}
	else if (symNow.type == 22 && GlobalDef == true)
	{
		if ((GVarMap.count(symNow.name)) == 1)
		{
			printf("error in VarDef() GVarMap");
			throw "Error";
		}
		struct VarItem *tempVarItem = (struct VarItem *)malloc(sizeof(struct VarItem));
		tempVarItem->isConst = false;
		tempVarItem->registerNum = ++GVarMapst;
		tempVarItem->globalNum = 0;
		tempVarItem->dimension = 0;
		string tempName = symNow.name;
		int tempDimesion = 0;
		if (sym[symst].type == 72)
		{
			symNow = sym[symst++];
		}
		while (symNow.type == 72)
		{
			symNow = sym[symst++];
			tempVarItem->d[tempDimesion] = GlobalAddExp();
			tempDimesion++;
			symNow = sym[symst++];
			if (symNow.type != 73)
			{
				printf("error in ConstDef 数组 ]");
				throw "Error";
			}
			if (sym[symst].type == 72)
			{
				symNow = sym[symst++];
			}
		}
		if (tempDimesion != 0) //是数组
		{
			tempVarItem->dimension = tempDimesion;
			int arraySize = 1;
			for (int i = 0; i < tempDimesion; i++)
			{
				arraySize *= tempVarItem->d[i];
			}
			tempVarItem->arraySize = arraySize;
			GVarMap[tempName] = *tempVarItem;
			arrayDecl = tempVarItem; //把当前的数组变量信息留到InitVal用
			if (sym[symst].type == 54 || sym[symst].type == 53)
			{
				fprintf(fpout, "@x%d = dso_local global [%d x i32] zeroinitializer\n", GVarMapst, arraySize);
				return 0;
			}
			symNow = sym[symst++];
			if (symNow.type != 52)
			{
				printf("error in VarDef() Global array '=' ");
				throw "Error";
			}
			baseNum = 0;
			Offset = 0;
			symNow = sym[symst++];
			arrayDef = true;
			memset(tempArr, 0, 1000*sizeof(int));
			if(arraySize>=950){
				printf("tempArr数组开小了");
			}
			ret = InitVal();
			fprintf(fpout, "@x%d = dso_local global [%d x i32] [",GVarMapst, arraySize);
			for(int i=0;i<arraySize-1;i++){
				fprintf(fpout, "i32 %d, ", tempArr[i]);
			}
			fprintf(fpout,"i32 %d]\n",tempArr[arraySize-1]);
		}
		else
		{ //不是数组
			if (sym[symst].type == 54 || sym[symst].type == 53)
			{ //没有初始化
				GVarMap[tempName] = *tempVarItem;
				fprintf(fpout, "@x%d = dso_local global i32 %d\n", GVarMapst, tempVarItem->globalNum);
				return 0;
			}
			symNow = sym[symst++];
			if (symNow.type != 52)
			{
				printf("error in VarDef() Global '=' ");
				throw "Error";
			}
			symNow = sym[symst++];
			int resultNum = InitVal();
			tempVarItem->globalNum = resultNum;
			GVarMap[tempName] = *tempVarItem;
			fprintf(fpout, "@x%d = dso_local global i32 %d\n", GVarMapst, resultNum);
		}
	}
	else
	{
		printf("error in VarDef()");
		throw "Error";
	}
	return 0;
}

int InitVal()
{
	if (GlobalDef == false)
	{
		if (arrayDef)
		{
			if (symNow.type == 57)
			{ //花括号  数组初始化
				symNow = sym[symst++];
				baseNum++;
				if (symNow.type == 58)
				{ //xxx = {}
					return 0;
				}
				else
				{
					InitVal();
					if (sym[symst].type == 53)
					{
						symNow = sym[symst++];
					}
					int savedIndex = Offset;
					while (symNow.type == 53)
					{ //同一个花括号内，逗号右移一位
						symNow = sym[symst++];
						int temp = 1;
						for (int i = arrayDecl->dimension - 1; i >= baseNum; i--)
						{
							temp *= arrayDecl->d[i];
						}
						Offset += temp;
						InitVal();
						if (sym[symst].type == 53)
						{
							symNow = sym[symst++];
						}
					}
					symNow = sym[symst++];
					if (symNow.type != 58)
					{
						printf("error in InitVal 数组 }");
						throw "Error";
					}
					Offset = savedIndex;
					baseNum--;
				}
			}
			else
			{ //ConstInitVal -> ConstExp
				fprintf(fpout, "    %%x%d = getelementptr [%d x i32], [%d x i32]* %%x%d, i32 0, i32 %d\n", ++VarMapSt, arrayDecl->arraySize, arrayDecl->arraySize, arrayDecl->registerNum, Offset);
				int tempVarSt = VarMapSt;
				int tempAns = Exp();
				tempExpStack = &ExpStack.top();
				if (arrayDecl->registerNum < 9999)
				{
					if (tempExpStack->type == 1)
					{
						fprintf(fpout, "    store i32 %d, i32* %%x%d\n", tempExpStack->value, tempVarSt);
					}
					else if (tempExpStack->type == 3)
					{
						fprintf(fpout, "    store i32 %%x%d, i32* %%x%d\n", tempExpStack->value, tempVarSt);
					}
					else
					{
						printf("error in InitVal 数组");
						throw "Error";
						return ret;
					}
				}
				ExpStack.pop();
				return 0;
			}
		}
		else
		{ //不是数组
			int tempAns = Exp();
			return 0;
		}
	}
	else //Global
	{
		if (arrayDef)
		{
			if (symNow.type == 57)
			{
				baseNum++;
				symNow = sym[symst++];
				if (symNow.type == 58)
				{ //xxx = {}
					return 0;
				}
				else
				{
					InitVal();
					if (sym[symst].type == 53)
					{
						symNow = sym[symst++];
					}
					int savedIndex = Offset;
					while (symNow.type == 53)
					{ //同一个花括号内，逗号右移一位
						symNow = sym[symst++];
						int temp = 1;
						for (int i = arrayDecl->dimension - 1; i >= baseNum; i--)
						{
							temp *= arrayDecl->d[i];
						}
						Offset += temp;
						InitVal();
						if (sym[symst].type == 53)
						{
							symNow = sym[symst++];
						}
					}
					symNow = sym[symst++];
					if (symNow.type != 58)
					{
						printf("error in InitVal Global数组 }");
						throw "Error";
					}
					Offset = savedIndex;
					baseNum--;
				}
			}
			else{
				VarInInit = false;
				int tempVarSt = VarMapSt;
				int tempAns = GlobalAddExp();
				if (VarInInit)
				{ //全局变量初始化不能用变量
					printf("全局变量初始化不能用变量");
					throw "Error";
				}
				tempArr[Offset] = tempAns;
				return 0;
			}
		}
		else
		{
			VarInInit = false;
			int result = GlobalAddExp();
			if (VarInInit)
			{ //全局变量初始化不能用变量
				throw "Error";
			}
			return result;
		}
	}
}
int FuncDef()
{
	ret = FuncType();
	// if (ret != 0)
	// 	return ret;
	symNow = sym[symst++];
	ret = Ident();
	// if (ret != 0)
	// 	return ret;
	symNow = sym[symst++];
	if (symNow.type != 55)
	{
		printf("error in FuncDef '('");
		throw "Error";
	}
	fprintf(fpout, "(");
	symNow = sym[symst++];
	if (symNow.type != 56)
	{
		printf("error in FuncDef ')'");
		throw "Error";
	}
	fprintf(fpout, ") {");
	symNow = sym[symst++];
	ret = Block();
	fprintf(fpout, "}");
	// if (ret != 0)
	// 	return ret;
	return 0;
}
int FuncType()
{
	if (symNow.type != 1)
	{
		printf("error in FuncType");
		throw "Error";
	}
	fprintf(fpout, "i32 ");
	return 0;
}
int Ident()
{ //只用于FuncDef
	if (symNow.type == 2)
	{
		fprintf(fpout, "@main");
		return 0;
	}
	else if (symNow.type == 22)
	{
	}
	else
	{
		printf("error in Ident");
		throw "Error";
	}
	return 0;
}
int Block()
{
	if (symNow.type != 57)
	{
		printf("error in Block '{'");
		throw "Error";
	}
	fprintf(fpout, "\n");
	symNow = sym[symst++];
	VarMapList.push_back(BVarMap);
	BVarMap.clear();
	while (symNow.type == 4 || symNow.type == 1 ||
		   symNow.type == 3 || symNow.type == 54 ||
		   symNow.type == 22 || symNow.type == 5 ||
		   (symNow.type >= 11 && symNow.type <= 20) ||
		   symNow.type == 57 || symNow.type == 7 ||
		   symNow.type == 8 || symNow.type == 9)
	{
		ret = BlockItem();
		symNow = sym[symst++];
	}
	if (symNow.type != 58)
	{
		printf("error in Block '}'");
		throw "Error";
	}
	if (!VarMapList.empty())
	{
		BVarMap = VarMapList.back();
		VarMapList.pop_back();
	}

	//fprintf(fpout, "\n");
	return 0;
}
int BlockItem()
{
	if (symNow.type == 4 || symNow.type == 1)
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
	if (symNow.type == 3)
	{
		symNow = sym[symst++];
		Exp();
		tempExpStack = &ExpStack.top();
		fprintf(fpout, "    ret");
		if (tempExpStack->type == 1)
		{
			fprintf(fpout, " i32 %d", tempExpStack->value);
		}
		else if (tempExpStack->type == 3)
		{
			fprintf(fpout, " i32 %%x%d", tempExpStack->value);
		}
		else
		{
			printf("error in Stmt");
			throw "Error";
			return ret;
		}
		ExpStack.pop();
		symNow = sym[symst++];
		if (symNow.type != 54)
		{
			printf("error in Stmt ';'");
			throw "Error";
		}
		fprintf(fpout, "\n");
		return 0;
	}
	else if (symNow.type == 54)
	{
		return 0;
	}
	else if (symNow.type == 22)
	{
		LvalIsConst = false;
		int retRegister = LVal();
		if (LvalIsConst)
		{
			printf("error in Stmt LvalIsConst");
			throw "Error";
		}
		symNow = sym[symst++];
		if (symNow.type != 52)
		{ //Stmt         ->  [Exp] ';'
			symNow = sym[symst - 2];
			symst -= 1;
			Exp();
			//ExpStack.pop();
			symNow = sym[symst++];
			if (symNow.type != 54)
			{
				printf("error in Stmt ';'");
				throw "Error";
			}
			return 0;
		}

		symNow = sym[symst++];
		int tempAns = Exp();
		symNow = sym[symst++];
		if (retRegister <= 9999)
		{
			if (tempExpStack->type == 1)
			{
				fprintf(fpout, "    store i32 %d, i32* %%x%d\n", tempExpStack->value, retRegister);
			}
			else if (tempExpStack->type == 3)
			{
				fprintf(fpout, "    store i32 %%x%d, i32* %%x%d\n", tempExpStack->value, retRegister);
			}
			else
			{
				printf("error in Stmt");
				throw "Error";
				return ret;
			}
		}
		else
		{
			if (tempExpStack->type == 1)
			{
				fprintf(fpout, "    store i32 %d, i32* @x%d\n", tempExpStack->value, retRegister);
			}
			else if (tempExpStack->type == 3)
			{
				fprintf(fpout, "    store i32 %%x%d, i32* @x%d\n", tempExpStack->value, retRegister);
			}
			else
			{
				printf("error in Stmt");
				throw "Error";
				return ret;
			}
		}

		if (symNow.type != 54)
		{
			printf("error in Stmt ';'");
			throw "Error";
		}
		return 0;
	}
	else if (symNow.type == 5)
	{ //条件语句
		symNow = sym[symst++];
		if (symNow.type != 55)
		{
			printf("error in Stmt (");
			throw "Error";
		}
		symNow = sym[symst++];
		Cond();
		tempExpStack = &ExpStack.top();
		fprintf(fpout, "    br i1 %%x%d, label %%t_%d, label %%f_%d\n\n", tempExpStack->value, condCount, (condCount + 1));
		condCountMap[true] = condCount;
		condCountTrueStack.push(condCount);
		condCountMap[false] = condCount + 1;
		condCountFalseStack.push(condCount + 1);
		condCount += 2;
		// fprintf(fpout,"\n");
		symNow = sym[symst++];
		if (symNow.type != 56)
		{
			printf("error in Stmt )");
			throw "Error";
		}
		symNow = sym[symst++];
		//fprintf(fpout,"Stmt If \n");
		int tem = condCountTrueStack.top();
		condCountTrueStack.pop();
		fprintf(fpout, "t_%d:\n", tem);
		Stmt();
		fprintf(fpout, "    br label %%m_%d\n", mainCount);
		fprintf(fpout, "\n");
		if (sym[symst].type == 6)
		{
			symNow = sym[symst++];
		}
		if (symNow.type == 6)
		{
			symNow = sym[symst++];
			//fprintf(fpout,"Stmt Else \n");
			int tem = condCountFalseStack.top();
			condCountFalseStack.pop();
			fprintf(fpout, "f_%d:\n", tem);
			Stmt();
			fprintf(fpout, "    br label %%m_%d\n", mainCount);
			fprintf(fpout, "\n");
		}
		if (!condCountTrueStack.empty())
		{
			int tem = condCountTrueStack.top();
			condCountTrueStack.pop();
			fprintf(fpout, "t_%d:\n", tem);
			fprintf(fpout, "    br label %%m_%d\n", mainCount);
		}
		if (!condCountFalseStack.empty())
		{
			int tem = condCountFalseStack.top();
			condCountFalseStack.pop();
			fprintf(fpout, "f_%d:\n", tem);
			fprintf(fpout, "    br label %%m_%d\n\n", mainCount);
		}
		fprintf(fpout, "m_%d:\n", mainCount);
		mainCount++;
		return 0;
	}
	else if (symNow.type == 7)
	{ //循环
		symNow = sym[symst++];
		if (symNow.type != 55)
		{
			printf("error in Stmt (");
			throw "Error";
		}
		fprintf(fpout, "    br label %%c_%d\n\n", whileCondCount);
		continueCountTrueStack.push(whileCondCount);
		symNow = sym[symst++];
		fprintf(fpout, "c_%d:\n", whileCondCount);
		int tempWhileCount = whileCondCount;
		whileCondCount++;
		Cond();
		tempExpStack = &ExpStack.top();
		fprintf(fpout, "    br i1 %%x%d, label %%t_%d, label %%f_%d\n\n", tempExpStack->value, condCount, (condCount + 1));
		condCountMap[true] = condCount;
		whileCountTrueStack.push(condCount);
		condCountMap[false] = condCount + 1;
		whileCountFalseStack.push(condCount + 1);
		condCount += 2;
		symNow = sym[symst++];
		if (symNow.type != 56)
		{
			printf("error in Stmt )");
			throw "Error";
		}
		symNow = sym[symst++];
		int tem = whileCountTrueStack.top();
		whileCountTrueStack.pop();
		fprintf(fpout, "t_%d:\n", tem);
		Stmt();
		continueCountTrueStack.pop();
		fprintf(fpout, "    br label %%c_%d\n", tempWhileCount);
		if (!whileCountTrueStack.empty())
		{
			int tem = whileCountTrueStack.top();
			whileCountTrueStack.pop();
			fprintf(fpout, "t_%d:\n", tem);
			fprintf(fpout, "    br label %%m_%d\n", mainCount);
		}
		if (!whileCountFalseStack.empty())
		{
			int tem = whileCountFalseStack.top();
			whileCountFalseStack.pop();
			fprintf(fpout, "f_%d:\n", tem);
			fprintf(fpout, "    br label %%m_%d\n\n", mainCount);
		}
		fprintf(fpout, "m_%d:\n", mainCount);
		mainCount++;
		return 0;
	}
	else if (symNow.type == 8)
	{ //break
		symNow = sym[symst++];
		if (symNow.type != 54)
		{
			printf("error in Stmt Break ';'");
			throw "Error";
		}
		int tem = whileCountFalseStack.top();
		fprintf(fpout, "    br label %%f_%d\n", tem);
	}
	else if (symNow.type == 9)
	{ //continue
		symNow = sym[symst++];
		if (symNow.type != 54)
		{
			printf("error in Stmt Continue ';'");
			throw "Error";
		}
		int tem = continueCountTrueStack.top();
		fprintf(fpout, "    br label %%c_%d\n", tem);
	}
	else if (symNow.type == 57)
	{ //Block
		Block();
	}
	else
	{
		Exp();
		//ExpStack.pop();
		symNow = sym[symst++];
		if (symNow.type != 54)
		{
			printf("error in Stmt ';'");
			throw "Error";
		}
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
	// if (ret != 0)
	// {
	// 	return ret;
	// }
	return 0;
}
int AddExp()
{
	ret = MulExp();
	// if (ret != 0)
	// 	return ret;
	if (sym[symst].type == 59 || sym[symst].type == 62)
	{ //读后面一个词，判断是否正确
		symNow = sym[symst++];
	}
	while (symNow.type == 59 || symNow.type == 62)
	{
		struct symType tempSym = symNow;
		symNow = sym[symst++];
		if (tempSym.type == 59)
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
		if (sym[symst].type == 59 || sym[symst].type == 62)
		{ //读后面一个词，判断是否正确
			symNow = sym[symst++];
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
	if (sym[symst].type == 60 || sym[symst].type == 61 || sym[symst].type == 63)
	{ //读后面一个词，判断是否正确
		symNow = sym[symst++];
	}
	while (symNow.type == 60 || symNow.type == 61 || symNow.type == 63)
	{
		struct symType tempSym = symNow;
		symNow = sym[symst++];
		if (tempSym.type == 60)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 2;
			tempExpStack->value = 3; //是乘法
			ExpStack.push(*tempExpStack);
		}
		else if (tempSym.type == 61)
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
		if (sym[symst].type == 60 || sym[symst].type == 61 || sym[symst].type == 63)
		{ //读后面一个词，判断是否正确
			symNow = sym[symst++];
		}
	}
	return 0;
}
int UnaryExp()
{
	//UnaryOp()
	if (symNow.type == 59 || symNow.type == 62 || symNow.type == 71)
	{
		UnaryOp();
		symNow = sym[symst++];
		UnaryExp();

		OperationUnaryOp();
	}
	else if (symNow.type >= 11 && symNow.type <= 20)
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
	if (symNow.type == 59 || symNow.type == 62 || symNow.type == 71)
	{
		if (symNow.type == 59)
		{ //多余一个+-号，则以此来判断Number的正负
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 4;
			tempExpStack->value = 1; //是正号
			ExpStack.push(*tempExpStack);
		}
		else if (symNow.type == 62)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 4;
			tempExpStack->value = 2; //是负号
			ExpStack.push(*tempExpStack);
		}
		else
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 4;
			tempExpStack->value = 3; //是Not,即！
			ExpStack.push(*tempExpStack);
		}
	}
}
int PrimaryExp()
{
	if (symNow.type == 55)
	{
		symNow = sym[symst++];
		Exp();
		symNow = sym[symst++];
		if (symNow.type != 56)
		{
			printf("error in PrimaryExp() RPar");
			throw "Error";
			return ret;
		}
	}
	else if (symNow.type == 21)
	{
		//Number()
		int retPriNum;
		retPriNum = symNow.value;
		tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
		tempExpStack->type = 1;
		tempExpStack->value = retPriNum;
		ExpStack.push(*tempExpStack);
	}
	else if (symNow.type == 22)
	{
		IsGlobalVal = false;
		int LvalRegister = LVal();
		tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
		tempExpStack->type = 3;
		tempExpStack->value = ++VarMapSt;
		ExpStack.push(*tempExpStack);
		if (IsGlobalVal)
		{
			fprintf(fpout, "    %%x%d = load i32, i32* @x%d\n", tempExpStack->value, LvalRegister);
		}
		else
		{
			fprintf(fpout, "    %%x%d = load i32, i32* %%x%d\n", tempExpStack->value, LvalRegister);
		}
	}
	else
	{
		printf("error in PrimaryExp()");
		throw "Error";
		return ret;
	}
	if (ret != 0)
	{
		return ret;
	}
	return 0;
}
int LVal()
{
	//检查是否是已经定义的变量,现在在当前的BVarMap找
	bool declared = false;
	varIt = BVarMap.find(symNow.name);
	if (varIt != BVarMap.end())
	{
		declared = true;
	}

	//找不到了，找外部的Map
	for (VarMapListIt = VarMapList.rbegin(); VarMapListIt != VarMapList.rend(); ++VarMapListIt)
	{
		if (declared)
		{ //在BVarMap已经找到了
			break;
		}
		varIt = (*VarMapListIt).find(symNow.name);
		if (varIt != (*VarMapListIt).end())
		{
			declared = true;
		}
		else
		{
			continue;
		}
	}
	if (!declared)
	{ //还是没找到，在全局变量找
		varIt = GVarMap.find(symNow.name);
		if (varIt != GVarMap.end())
		{
			IsGlobalVal = true;
			declared = true;
		}
	}
	if (!declared)
	{
		printf("error Lval don't exist");
		throw "Error";
	}
	bool isArray = false;
	if (sym[symst].type == 72)
	{ //{'[' Exp ']'}
		symNow = sym[symst++];
		isArray = true;
	}
	int tempDimension = (*varIt).second.dimension;
	int base = 0;	   //括号数
	int tempindex = 0; //偏移数
	int temp = 1;	   //偏移基数
	int bracketCheck = 0;  //检测维数是否正确
	fprintf(fpout,"    %%x%d = alloca i32\n",++VarMapSt);
	fprintf(fpout,"    store i32 0, i32* %%x%d\n",VarMapSt);
	fprintf(fpout,"    %%x%d = load i32, i32* %%x%d\n",++VarMapSt,VarMapSt);
	while (symNow.type == 72)
	{
		symNow = sym[symst++];
		base++;
		bracketCheck++;
		temp = 1;
		map<string, struct VarItem>::iterator savedVarIt = varIt;
		int result = Exp();
		varIt = savedVarIt;
		tempExpStack = &ExpStack.top();
		//fprintf(fpout,"Lval Exp here\n");
		for (int i = tempDimension - 1; i >= base; i--)
		{
			temp *= (*varIt).second.d[i];
		}

		//tempindex += result * temp;
		if (tempExpStack->type == 1)
		{
			fprintf(fpout, "    %%x%d = mul i32 %d, %d\n", ++VarMapSt, tempExpStack->value,temp);
			fprintf(fpout, "    %%x%d = add i32 %%x%d, %%x%d\n", ++VarMapSt, VarMapSt-1,VarMapSt);
		}
		else if (tempExpStack->type == 3)
		{
			fprintf(fpout, "    %%x%d = mul i32 %%x%d, %d\n", ++VarMapSt, tempExpStack->value,temp);
			fprintf(fpout, "    %%x%d = add i32 %%x%d, %%x%d\n", ++VarMapSt, VarMapSt-1,VarMapSt);
		}
		else
		{
			printf("error in Lval() tempindex");
			throw "Error";
		}



		symNow = sym[symst++];
		if (symNow.type != 73)
		{
			printf("error in Lval ]");
			throw "Error";
		}
		if (sym[symst].type == 72)
		{ //{'[' Exp ']'}
			symNow = sym[symst++];
		}
		ExpStack.pop();
	}
	if(bracketCheck != (*varIt).second.dimension){
		printf("Lval数组的维数错误");
		throw "Error";
	}
	if ((*varIt).second.isConst)
	{ //这个变量是常量
		LvalIsConst = true;
	}
	else //这个变量不是常量，如果它在常量的初始化式子中，则非法。
	{
		VarInInit = true;
	}

	if (isArray)
	{
		if(!IsGlobalVal)
			fprintf(fpout, "    %%x%d = getelementptr [%d x i32], [%d x i32]* %%x%d, i32 0, i32 %%x%d\n", ++VarMapSt, (*varIt).second.arraySize, (*varIt).second.arraySize, (*varIt).second.registerNum, VarMapSt);
		else fprintf(fpout, "    %%x%d = getelementptr [%d x i32], [%d x i32]* @x%d, i32 0, i32 %%x%d\n", ++VarMapSt, (*varIt).second.arraySize, (*varIt).second.arraySize, (*varIt).second.registerNum, VarMapSt);
		IsGlobalVal = false;  //防止load的时候使用全局变量的输出
		return VarMapSt;
	}
	// if ((*varIt).second.isConst)
	// { //这个变量是常量
	// 	LvalIsConst = true;
	// }
	// else //这个变量不是常量，如果它在常量的初始化式子中，则非法。
	// {
	// 	VarInInit = true;
	// }
	return (*varIt).second.registerNum; //返回寄存器数字
}
void Cond()
{
	condHasIcmp = false;
	LOrExp();
	if (!condHasIcmp)
	{
		struct ExpElem num = ExpStack.top();
		ExpStack.pop();
		if (num.type != 1 && num.type != 3)
		{
			printf("error in Cond");
			throw "Error";
		}

		// if (ExpStack.empty())
		// {
		// 	ExpStack.push(num);
		// 	return;
		// }

		if (num.type == 1)
		{
			fprintf(fpout, "    %%x%d = icmp ne i32 %d, 0\n", ++VarMapSt, num.value);
		}
		else
		{
			fprintf(fpout, "    %%x%d = icmp ne i32 %%x%d, 0\n", ++VarMapSt, num.value);
		}
		num.value = VarMapSt;
		num.type = 3;
		ExpStack.push(num);
		tempExpStack = &ExpStack.top();
	}
}
void LOrExp()
{
	LAndExp();
	if (sym[symst].type == 66)
		symNow = sym[symst++];
	while (symNow.type == 66)
	{
		struct symType tempSym = symNow;
		symNow = sym[symst++];
		if (tempSym.type == 66)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 2;
			tempExpStack->value = 7; //是||
			ExpStack.push(*tempExpStack);
		}
		LAndExp();
		Operation();
		//fprintf(fpout,"block LOrd\n ");
		// int tem = condCountFalseStack.top();
		// condCountFalseStack.pop();
		// fprintf(fpout,"%d_f:\n", tem);
		// LAndExp();
		// if(!condCountFalseStack.empty()){
		// 	int tem2 = condCountFalseStack.top();
		// 	condCountFalseStack.pop();
		// 	int tem3 = condCountFalseStack.top();
		// 	fprintf(fpout,"%d_f:\n", tem2);
		// 	fprintf(fpout,"    br label %%x%d_f\n",tem3);
		// }
		if (sym[symst].type == 66)
			symNow = sym[symst++];
	}
	return;
}
void LAndExp()
{
	EqExp();
	if (sym[symst].type == 67)
		symNow = sym[symst++];
	while (symNow.type == 67)
	{
		struct symType tempSym = symNow;
		symNow = sym[symst++];
		//fprintf(fpout,"block LAnd\n ");
		if (tempSym.type == 67)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 2;
			tempExpStack->value = 6; //是&&
			ExpStack.push(*tempExpStack);
		}
		EqExp();
		Operation();
		// int tem = condCountTrueStack.top();
		// condCountTrueStack.pop();
		// fprintf(fpout,"%d_t:\n", tem);
		// EqExp();
		// if(!condCountTrueStack.empty()){
		// 	int tem2 = condCountTrueStack.top();
		// 	condCountTrueStack.pop();
		// 	int tem3 = condCountTrueStack.top();
		// 	fprintf(fpout,"%d_t:\n", tem2);
		// 	fprintf(fpout,"    br label %%x%d_t\n",tem3);
		// }
		if (sym[symst].type == 67)
			symNow = sym[symst++];
	}
}
void EqExp()
{
	RelExp();
	if (sym[symst].type == 51 || sym[symst].type == 68)
		symNow = sym[symst++];
	while (symNow.type == 51 || symNow.type == 68)
	{
		struct symType tempSym = symNow;
		symNow = sym[symst++];
		if (tempSym.type == 51)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 5;
			tempExpStack->value = 1; //是相等
			ExpStack.push(*tempExpStack);
		}
		else
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 5;
			tempExpStack->value = 2; //是不相等
			ExpStack.push(*tempExpStack);
		}
		EqExp();
		OperationCond();
		condHasIcmp = true;
		if (sym[symst].type == 51 || sym[symst].type == 68)
			symNow = sym[symst++];
	}
}
void RelExp()
{
	AddExp();
	if (sym[symst].type == 64 || sym[symst].type == 65 || sym[symst].type == 69 || sym[symst].type == 70)
		symNow = sym[symst++];
	while (symNow.type == 64 || symNow.type == 65 || symNow.type == 69 || symNow.type == 70)
	{
		struct symType tempSym = symNow;
		symNow = sym[symst++];
		if (tempSym.type == 64)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 5;
			tempExpStack->value = 3; //是小于
			ExpStack.push(*tempExpStack);
		}
		else if (tempSym.type == 65)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 5;
			tempExpStack->value = 4; //是大于
			ExpStack.push(*tempExpStack);
		}
		else if (tempSym.type == 69)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 5;
			tempExpStack->value = 5; //是小于等于
			ExpStack.push(*tempExpStack);
		}
		else
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 5;
			tempExpStack->value = 6; //是大于等于
			ExpStack.push(*tempExpStack);
		}
		AddExp();
		OperationCond();
		condHasIcmp = true;
		if (sym[symst].type == 64 || sym[symst].type == 65 || sym[symst].type == 69 || sym[symst].type == 70)
			symNow = sym[symst++];
	}
}
void FuncCall()
{
	funcIt = FuncMap.find(symNow.name);
	if (funcIt == FuncMap.end())
	{
		printf("error in FuncCall");
		throw "Error";
	}
	symNow = sym[symst++];
	if (symNow.type != 55)
	{
		printf("error in FuncCall '('");
		throw "Error";
	}
	symNow = sym[symst++];
	// if (symNow.type != 55)
	// {
	int paramsNum = (*funcIt).second.paramsNum;
	while (paramsNum > 0)
	{
		FuncRParams();
		paramsNum--;
		symNow = sym[symst++];
		if (paramsNum == 0)
			break;
		if (symNow.type != 53)
		{
			printf("error in FuncCall FuncParams");
			throw "Error";
		}
		symNow = sym[symst++];
	}
	//}

	if (symNow.type != 56)
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
		fprintf(fpout, "    %%x%d = call i32 %s", tempExpStack->value, (*funcIt).second.funcName.c_str());
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
			fprintf(fpout, "i32 %%x%d", ExpStack.top().value);
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

	fprintf(fpout, "    %%x%d = ", tempExpStack->value);
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
	case 6:
		fprintf(fpout, "and i1 ");
		break;
	case 7:
		fprintf(fpout, "or i1 ");
		break;
	}
	if (num1.type == 1)
	{
		fprintf(fpout, "%d,", num1.value);
	}
	else
	{
		fprintf(fpout, "%%x%d,", num1.value);
	}
	if (num2.type == 1)
	{
		fprintf(fpout, " %d", num2.value);
	}
	else
	{
		fprintf(fpout, " %%x%d", num2.value);
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
			fprintf(fpout, "    %%x%d = sub i32 0, %d\n", ++VarMapSt, num.value);
		}
		else
		{
			fprintf(fpout, "    %%x%d = sub i32 0, %%x%d\n", ++VarMapSt, num.value);
		}
		num.type = 3;
		num.value = VarMapSt;
	}
	else if (op.value == 3)
	{
		if (num.type == 1)
		{
			fprintf(fpout, "    %%x%d = icmp eq i32 %d, 0\n", ++VarMapSt, num.value);
		}
		else
		{
			fprintf(fpout, "    %%x%d = icmp eq i32 %%x%d, 0\n", ++VarMapSt, num.value);
		}
		num.value = VarMapSt;
		fprintf(fpout, "    %%x%d = zext i1 %%x%d to i32\n", ++VarMapSt, num.value);
		num.type = 3;
		num.value_1 = num.value;
		num.value = VarMapSt;
	}
	ExpStack.push(num);
	tempExpStack = &ExpStack.top();
}
void OperationCond()
{
	struct ExpElem num1, op, num2;
	num2 = ExpStack.top();
	if (num2.type != 1 && num2.type != 3)
	{
		printf("errro in Operation");
		throw "Error";
	}
	ExpStack.pop();
	op = ExpStack.top();
	if (op.type != 5)
	{
		printf("errro in OperationCond");
		throw "Error";
	}
	ExpStack.pop();
	num1 = ExpStack.top();
	if (num1.type != 1 && num1.type != 3)
	{
		printf("error in Operation");
		throw "Error";
	}
	ExpStack.pop();

	tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
	tempExpStack->type = 3;
	tempExpStack->value = ++VarMapSt;
	ExpStack.push(*tempExpStack);

	fprintf(fpout, "    %%x%d = ", tempExpStack->value);
	switch (op.value)
	{
	case 1:
		fprintf(fpout, "icmp eq i32 ");
		break;
	case 2:
		fprintf(fpout, "icmp ne i32 ");
		break;
	case 3:
		fprintf(fpout, "icmp slt i32 ");
		break;
	case 4:
		fprintf(fpout, "icmp sgt i32 ");
		break;
	case 5:
		fprintf(fpout, "icmp sle i32 ");
		break;
	case 6:
		fprintf(fpout, "icmp sge i32 ");
		break;
	}
	if (num1.type == 1)
	{
		fprintf(fpout, "%d,", num1.value);
	}
	else
	{
		fprintf(fpout, "%%x%d,", num1.value);
	}
	if (num2.type == 1)
	{
		fprintf(fpout, " %d", num2.value);
	}
	else
	{
		fprintf(fpout, " %%x%d", num2.value);
	}
	fprintf(fpout, "\n");
	// fprintf(fpout,"    br i1 %%x%d, label %%x%d_t, label %%x%d_f\n",tempExpStack->value,condCount,(condCount+1));
	// condCountMap[true] = condCount;
	// condCountTrueStack.push(condCount);
	// condCountMap[false] = condCount+1;
	// condCountFalseStack.push(condCount+1);
	// condCount+=2;
	// fprintf(fpout,"\n");
}

int GlobalAddExp()
{
	int RetNum = GlobalMulExp();
	if (sym[symst].type == 59 || sym[symst].type == 62)
	{ //读后面一个词，判断是否正确
		symNow = sym[symst++];
	}
	while (symNow.type == 59 || symNow.type == 62)
	{
		struct symType tempSymNow = symNow;
		symNow = sym[symst++];
		if (tempSymNow.type == 59)
		{
			RetNum += GlobalMulExp();
		}
		else
		{
			RetNum -= GlobalMulExp();
		}
		if (sym[symst].type == 59 || sym[symst].type == 62)
		{ //读后面一个词，判断是否正确
			symNow = sym[symst++];
		}
	}
	return RetNum;
}
int GlobalMulExp()
{
	int RetNum = GlobalUnaryExp();
	if (sym[symst].type == 60 || sym[symst].type == 61 || sym[symst].type == 63)
	{ //读后面一个词，判断是否正确
		symNow = sym[symst++];
	}
	while (symNow.type == 60 || symNow.type == 61 || symNow.type == 63)
	{
		struct symType tempSymNow = symNow;
		symNow = sym[symst++];
		if (tempSymNow.type == 60)
		{
			RetNum *= GlobalUnaryExp();
		}
		else if (tempSymNow.type == 60)
		{
			RetNum /= GlobalUnaryExp();
		}
		else
		{
			RetNum %= GlobalUnaryExp();
		}
		if (sym[symst].type == 60 || sym[symst].type == 61 || sym[symst].type == 63)
		{ //读后面一个词，判断是否正确
			symNow = sym[symst++];
		}
	}
	return RetNum;
}
int GlobalUnaryExp()
{
	int RetNum;
	//UnaryOp()
	if (symNow.type == 59 || symNow.type == 62)
	{
		int PositiveNum = 1;
		if (symNow.type == 59)
		{ //多余一个+-号，则以此来判断Number的正负
			PositiveNum = PositiveNum;
		}
		else
		{
			PositiveNum = -PositiveNum;
		}
		symNow = sym[symst++];
		RetNum = GlobalUnaryExp();
		RetNum = RetNum * PositiveNum;
	}
	else
	{
		RetNum = GlobalPrimaryExp();
	}
	return RetNum;
}
int GlobalPrimaryExp()
{
	int RetNum;
	if (symNow.type == 55)
	{
		symNow = sym[symst++];
		RetNum = GlobalAddExp();
		symNow = sym[symst++];
		if (symNow.type != 56)
		{
			printf("error in GlobalPrimaryExp() )");
			throw "Error";
		}
		return RetNum;
	}
	else if (symNow.type == 21)
	{
		//Number()
		int retPriNum;
		retPriNum = symNow.value;
		return retPriNum;
	}
	else if (symNow.type == 22)
	{ //ident
		int retPriNum;
		bool declared = false;
		varIt = GVarMap.find(symNow.name);
		if (varIt != GVarMap.end())
		{
			declared = true;
		}
		else
		{
			printf("error in GlobalPrimaryExp() symNow.type == 22");
			throw "Error";
		}
		if ((*varIt).second.isConst)
		{ //这个变量是常量
			LvalIsConst = true;
		}
		else
		{
			printf("error in GlobalPrimaryExp() LvalIsConst");
			throw "Error";
		}
		retPriNum = (*varIt).second.globalNum;
		return retPriNum;
	}
	else
	{
		printf("error in GlobalPrimaryExp()");
		throw "Error";
	}
}
