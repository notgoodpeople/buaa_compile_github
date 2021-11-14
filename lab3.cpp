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
int sst = 0;   //��ʾ���Ӷ���λ�� sentenceStart
int tst = 0;   //��ʾ�ʶ���λ�� tokenStart
int symed = 0; //��ʾ�洢���ŵĴ�������λ��
int symst = 0; //��ʾ �洢���ŵĴ���ĵ�ǰ���?
char key[9][15] = {"int", "main", "return", "const"};
char keyOut[9][15] = {"Int", "Main", "Return", "Const"};
char funcCall[9][15] = {"getint", "putint", "getch", "putch"};
char funcCallOut[9][15] = {"Func(getint)", "Func(putint)", "Func(getch)", "Func(putch)"};
char sym[3005][20];
int ret = 0;		//��������ķ����?
int tempRetNum = 0; //EXP()ʽ���е���ʱ����ֵ
struct ExpElem
{
	int type; //1 Exp�е����֣�2 �������?3 �Ĵ�����4 UnaryOp
	/* 
	1�����ֵ�ֵ
	2��1�ӷ���2������3�˷���4������5ȡ��
	3���Ĵ����ı��?
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
	getToken();
	strcpy(token, sym[symst++]);
	ret = CompUnit();
	if (ret != 0)
		return ret;
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
	sprintf(sym[symed++], "Number(%d)", sum);
}
//�ڱ���ʽ���㣬Exp()�ຯ��ʱ ʹ�øú�������
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
			//Ident���߹ؼ���
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
				//�ж��Ƿ��ǹؼ���
				for (int i = 0; i <= 3; i++)
				{
					if (strcmp(token, key[i]) == 0)
					{
						strcpy(sym[symed++], keyOut[i]);
						iskey = 1;
					}
				}
				//�ж��Ƿ��ǵ��ú���
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
		return ret;
	strcpy(token, sym[symst++]);
	if (token[0] != 0)
	{
		printf("error in CompUnit");
		throw "Error";
	}
	return 0;
}
int Decl()
{
	if (strcmp(token, "Const") == 0)
	{
		strcpy(token, sym[symst++]);
		ret = ConstDecl();
		if(ret!=0){
			throw "Error";
		}
		return ret;
	}
	else
	{
		return VarDecl();
	}
}
int ConstDecl()
{
	//const��Decl()���Ѿ����?

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
			throw "Error";
			return ret;
		}
		strcpy(token, sym[symst++]);
	}
	if (strcmp(token, "Semicolon") != 0)
	{
		printf("error in ConstDecl ;");
		throw "Error";
		return 139;
	}
	return 0;
}
int Btype()
{
	if (strcmp(token, "Int") != 0)
	{
		printf("error in Btype()");
		throw "Error";
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
			throw "Error";
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
			throw "Error";
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
			throw "Error";
		}
		ExpStack.pop();
	}
	else
	{
		printf("error in ConstDef()");
		throw "Error";
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
			throw "Error";
			return ret;
		}
		strcpy(token, sym[symst++]);
	}
	if (strcmp(token, "Semicolon") != 0)
	{
		printf("error in VarDecl ;");
		throw "Error";
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
			throw "Error";
		}
		varIt = VarMap.find((string)token);
		if (varIt != VarMap.end())
		{
			printf("error in varDef()");
			throw "Error";
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
		if (ret != 0){
			throw "Error";
			return ret;
		}
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
			throw "Error";
		}
		ExpStack.pop();
	}
	else
	{
		printf("error in VarDef()");
		throw "Error";
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
		throw "Error";
		return 103;
	}
	fprintf(fpout, "(");
	strcpy(token, sym[symst++]);
	if (strcmp(token, "RPar") != 0)
	{
		printf("error in FuncDef ')'");
		throw "Error";
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
		throw "Error";
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
		throw "Error";
		return 141;
	}
	return 0;
}
int Block()
{
	if (strcmp(token, "LBrace") != 0)
	{
		printf("error in Block '{'");
		throw "Error";
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
		throw "Error";
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
			throw "Error";
		}
		ExpStack.pop();
		strcpy(token, sym[symst++]);
		if (strcmp(token, "Semicolon") != 0)
		{
			printf("error in Stmt ';'");
			throw "Error";
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
			throw "Error";
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
			throw "Error";
		}
		if (strcmp(token, "Semicolon") != 0)
		{
			printf("error in Stmt ';'");
			throw "Error";
			return 166;
		}
		fprintf(fpout, "\n");
		return 0;
	}
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
	if (strcmp(sym[symst], "Plus") == 0 || strcmp(sym[symst], "Minus") == 0)
	{ //������һ���ʣ��ж��Ƿ���ȷ
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
		if (strcmp(sym[symst], "Plus") == 0 || strcmp(sym[symst], "Minus") == 0)
		{ //������һ���ʣ��ж��Ƿ���ȷ
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
	{ //������һ���ʣ��ж��Ƿ���ȷ
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
			tempExpStack->value = 3; //�ǳ˷�
			ExpStack.push(*tempExpStack);
		}
		else if (strcmp(temptoken, "Div") == 0)
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
		if (strcmp(sym[symst], "Mult") == 0 || strcmp(sym[symst], "Div") == 0 || strcmp(sym[symst], "Surplus") == 0)
		{ //������һ���ʣ��ж��Ƿ���ȷ
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
	if (strcmp(token, "LPar") == 0)
	{
		strcpy(token, sym[symst++]);
		Exp();
		strcpy(token, sym[symst++]);
		if (strcmp(token, "RPar") != 0)
		{
			printf("error in PrimaryExp() RPar");
			throw "Error";
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
		throw "Error";
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
	return (*varIt).second.registerNum; //���ؼĴ�������
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
			fprintf(fpout, "i32* %%%d", ExpStack.top().value);
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
		throw "Error";
	}
	ExpStack.pop();
	op = ExpStack.top();
	if (op.type != 2)
	{
		printf("errro in Operation");
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
		throw "Error";
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
}
