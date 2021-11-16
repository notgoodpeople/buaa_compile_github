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
char str[1024];
char token[256];
int sst = 0;   //��ʾ���Ӷ���λ�� sentenceStart
int tst = 0;   //��ʾ�ʶ���λ�� tokenStart
int symed = 0; //��ʾ�洢���ŵĴ�������λ��
int symst = 0; //��ʾ �洢���ŵĴ���ĵ�ǰ��ȡ
char key[9][15] = {"int", "main", "return", "const", "if", "else"};
char keyOut[9][15] = {"Int", "Main", "Return", "Const", "If", "Else"};
char funcCall[9][15] = {"getint", "putint", "getch", "putch"};
char funcCallOut[9][15] = {"Func(getint)", "Func(putint)", "Func(getch)", "Func(putch)"};
struct symType{    //���ڷ����Ĵ�
	string name; 
	/*���ͣ�1~10 Ϊ�ؼ��֣�int��main��return��const��if��else
		11~20Ϊ�������ã�getint putint getch putch
		21 Number 22 Ident
		51~ ���� 51 == 52 = 53 , 54 ; 55 ( 56 ) 57 { 58 } 59 + 60 *
				61 / 62 - 63 % 64 < 65 >
	*/
	int value; //�����number��  �ᴢ��int�͵�ֵ
	int type;
}sym[1005];
struct symType symNow;
int ret = 0;		//�������ķ���ֵ
int tempRetNum = 0; //EXP()ʽ���е���ʱ����ֵ
struct ExpElem
{
	int type; //1 Exp�е����֣�2 �������3 �Ĵ�����4 UnaryOp
	/* 
	1�����ֵ�ֵ
	2��1�ӷ���2������3�˷���4������5ȡ��
	3���Ĵ����ı��
	4��1���� 2����
	*/
	int value;
};
struct ExpElem *tempExpStack;
stack<struct ExpElem> ExpStack; //�������ɼ���ֵLLVM IR��ջ
bool VarInInit = false;
bool LvalIsConst = false;
struct VarItem
{
	bool isConst;	 //�Ƿ��ǳ���
	int registerNum; //�Ĵ����ĺ���
};
map<string, struct VarItem>::iterator varIt; //Varmap����������
map<string, struct VarItem> VarMap;
int VarMapSt = 0; //��ǰ�¼Ĵ�����ֵ
struct FuncItem
{
	int RetType;		//������������ 1Ϊint 0Ϊvoid
	vector<int> params; //���������б�
	string funcName;	//LLVM IR�еĺ�����
	int paramsNum;     //������������
};
map<string, struct FuncItem> FuncMap;
map<string, struct FuncItem>::iterator funcIt; //Funcmap����������
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
	try{
		getToken();
		symNow = sym[symst++];
		ret = CompUnit();
		if (ret != 0)
			return ret;
	}catch(...){
		return 998;
	}
	return 0;
}

//����ת��
void ChangeTen(int n, char str[])
{ //��n������ת����10������
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
//�ڱ��ʽ���㣬Exp()�ຯ��ʱ ʹ�øú�������
void error()
{
	ret = 120;
	printf("\nExp() error");
}
//�ʷ�����
int getToken()
{
	int note = 0;
	tst = 0;
	while (fgets(str, 512, fpin) != NULL)
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
			//Ident���߹ؼ���
			else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
			{
				token[tst++] = ch;
				ch = str[++sst];
				while ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '\n')
				{
					if(ch == '\n'){
						fgets(str, 2000, fpin);
						sst=-1;
						ch = str[++sst];
					}
					else{
						token[tst++] = ch;
						ch = str[++sst];
					}
				}
				token[tst] = '\0';
				tst = 0;
				//�ж��Ƿ��ǹؼ���
				for (int i = 0; i <= 5; i++)
				{
					if (strcmp(token, key[i]) == 0)
					{
						sym[symed].name = keyOut[i];
						sym[symed++].type = i+1;
						iskey = 1;
					}
				}
				//�ж��Ƿ��ǵ��ú���
				for (int i = 0; i <= 3; i++)
				{
					if (strcmp(token, funcCall[i]) == 0)
					{
						sym[symed].name = funcCallOut[i];
						sym[symed++].type = i+11;
						iskey = 1;
					}
				}
				if (iskey != 1)
				{
					sym[symed].name = token;
					sym[symed++].type = 22;
				}
			}
			//Number��
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
					//16����
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
						//16����Number����
						else
						{
							return 16;
						}
					}
					//8����
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
			else if(ch=='<'){
				sym[symed].name = "Lt";
				sym[symed++].type = 64;
				sst++;
			}
			else if(ch=='>'){
				sym[symed].name = "Gt";
				sym[symed++].type = 65;
				sst++;
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
//��ʼ����������
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
//�﷨����
int CompUnit()
{
	initFunc();
	fprintf(fpout, "define dso_local ");
	ret = FuncDef();
	if (ret != 0)
		throw "Error";
	symNow = sym[symst++];
	if (symst!=symed+1)
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
	//const��Decl()���Ѿ����

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
	if (symNow.type == 22)
	{
		if ((VarMap.count(symNow.name)) == 1)
		{
			printf("error in ConstDef() Ident");
			throw "Error";
		}
		struct VarItem *tempVarItem = (struct VarItem *)malloc(sizeof(struct VarItem));
		tempVarItem->isConst = true;
		tempVarItem->registerNum = ++VarMapSt;
		VarMap[symNow.name] = *tempVarItem;

		fprintf(fpout, "    %%%d = alloca i32\n", VarMapSt);
		symNow = sym[symst++];
		if (symNow.type != 52)
		{
			printf("error in ConstDef() '=' ");
			throw "Error";
		}
		symNow = sym[symst++];
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
			fprintf(fpout, "    store i32 %%%d, i32* %%%d\n", tempExpStack->value, tempVarItem->registerNum);
		}
		else
		{
			printf("error in VarDef()");
			throw "Error";
			return ret;
		}
		ExpStack.pop();
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
	VarInInit = false;
	ConstExp();
	if(VarInInit){
		throw "Error";
	}
	return 0;
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
	if (symNow.type == 22)
	{
		if (VarMap.count(symNow.name) == 1)
		{
			printf("error in VarDef() Ident");
			throw "Error";
		}
		varIt = VarMap.find(symNow.name);
		if (varIt != VarMap.end())
		{
			printf("error in varDef()");
			error();
			return ret;
		}
		struct VarItem *tempVarItem = (struct VarItem *)malloc(sizeof(struct VarItem));
		tempVarItem->isConst = false;
		tempVarItem->registerNum = ++VarMapSt;
		VarMap[symNow.name] = *tempVarItem;

		fprintf(fpout, "    %%%d = alloca i32\n", VarMapSt);
		if (sym[symst].type == 52)
			symNow = sym[symst++];
		if (symNow.type != 52)
		{
			return 0;
		}
		symNow = sym[symst++];
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
			fprintf(fpout, "    store i32 %%%d, i32* %%%d\n", tempExpStack->value, tempVarItem->registerNum);
		}
		else
		{
			printf("error in VarDef()");
			throw "Error";
			return ret;
		}
		ExpStack.pop();
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
	int tempAns = Exp();
	return 0;
}
int FuncDef()
{
	ret = FuncType();
	if (ret != 0)
		return ret;
	symNow = sym[symst++];
	ret = Ident();
	if (ret != 0)
		return ret;
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
	fprintf(fpout, ")");
	symNow = sym[symst++];
	ret = Block();
	if (ret != 0)
		return ret;
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
{   //ֻ����FuncDef
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
	fprintf(fpout, "{\n");
	symNow = sym[symst++];
	while (symNow.type == 4 || symNow.type == 1 || \
	symNow.type == 3 || symNow.type == 54 || \
	symNow.type == 22||\
	(symNow.type>=11&&symNow.type<=20))
	{
		ret = BlockItem();
		if (ret != 0)
			return ret;
		symNow = sym[symst++];
	}
	if (symNow.type != 58)
	{
		printf("error in Block '}'");
		throw "Error";
	}
	fprintf(fpout, "}");
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
			fprintf(fpout, " i32 %%%d", tempExpStack->value);
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
		if(LvalIsConst){
			throw "Error";
		}
		symNow = sym[symst++];
		if (symNow.type != 52)
		{
			printf("error in Stmt '='");
			throw "Error";
		}

		symNow = sym[symst++];
		int tempAns = Exp();
		symNow = sym[symst++];
		if (tempExpStack->type == 1)
		{
			fprintf(fpout, "    store i32 %d, i32* %%%d\n", tempExpStack->value, retRegister);
		}
		else if (tempExpStack->type == 3)
		{
			fprintf(fpout, "    store i32 %%%d, i32* %%%d\n", tempExpStack->value, retRegister);
		}
		else
		{
			printf("error in Stmt");
			throw "Error";
			return ret;
		}
		if (symNow.type != 54)
		{
			printf("error in Stmt ';'");
			throw "Error";
		}
		return 0;
	}
//	else if{   //�������

//	}
	else
	{ //���޸�
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
	if (sym[symst].type == 59 || sym[symst].type == 62)
	{ //������һ���ʣ��ж��Ƿ���ȷ
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
			tempExpStack->value = 1; //�Ǽӷ�
			ExpStack.push(*tempExpStack);
		}
		else
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 2;
			tempExpStack->value = 2; //�Ǽ���
			ExpStack.push(*tempExpStack);
		}
		ret = MulExp();
		if (ret != 0)
			return ret;
		Operation();
		if (sym[symst].type == 59 || sym[symst].type == 62)
		{ //������һ���ʣ��ж��Ƿ���ȷ
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
	{ //������һ���ʣ��ж��Ƿ���ȷ
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
			tempExpStack->value = 3; //�ǳ˷�
			ExpStack.push(*tempExpStack);
		}
		else if (tempSym.type == 61)
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 2;
			tempExpStack->value = 4; //�ǳ���
			ExpStack.push(*tempExpStack);
		}
		else
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 2;
			tempExpStack->value = 5; //��ȡ��
			ExpStack.push(*tempExpStack);
		}
		RetNum = UnaryExp();
		if (ret != 0)
			return ret;
		Operation();
		if (sym[symst].type == 60 || sym[symst].type == 61 || sym[symst].type == 63)
		{ //������һ���ʣ��ж��Ƿ���ȷ
			symNow = sym[symst++];
		}
	}
	return 0;
}
int UnaryExp()
{
	//UnaryOp()
	if (symNow.type == 59 || symNow.type == 62)
	{
		UnaryOp();
		symNow = sym[symst++];
		UnaryExp();

		OperationUnaryOp();
	}
	else if (symNow.type>=11&&symNow.type<=20)
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
	if (symNow.type == 59 || symNow.type == 62)
	{
		if (symNow.type == 59)
		{ //����һ��+-�ţ����Դ����ж�Number������
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 4;
			tempExpStack->value = 1; //������
			ExpStack.push(*tempExpStack);
		}
		else
		{
			tempExpStack = (struct ExpElem *)malloc(sizeof(struct ExpElem));
			tempExpStack->type = 4;
			tempExpStack->value = 2; //�Ǹ���
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
	//����Ƿ����Ѿ�����ı���
	bool declared = false;
	varIt = VarMap.find(symNow.name);
	if (varIt != VarMap.end())
	{
		declared = true;
	}

	if ((*varIt).second.isConst)
	{     //��������ǳ���
		LvalIsConst = true;
	}
	else   //����������ǳ�����������ڳ����ĳ�ʼ��ʽ���У���Ƿ���
	{
		VarInInit = true;
	}
	return (*varIt).second.registerNum; //���ؼĴ�������
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
	if (symNow.type != 55)
	{
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
	}

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

	//�Ѿ��������ļӼ�����
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
	tempExpStack = &ExpStack.top();
}

