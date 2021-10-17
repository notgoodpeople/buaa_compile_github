#include<stdio.h>
#include<string.h>
#include<stdlib.h>
char str[256];
char token[20];
int sst=0;  //表示句子读的位置 sentenceStart
int tst=0;  //表示词读的位置 tokenStart
int symed=0; //表示存储符号的词组的最后位置 
int symst=0; //表示 存储符号的词组的当前读取 
char *key[7]={"int","main","return","break","continue","return"};
char *keyOut[7]={"Int","Main","Return","Break","Continue","Return"};
char sym[1005][20]; 
int ret=0;
FILE *fpin;
FILE *fpout;
int main(int argc,char *argv[]){
	fpout = fopen(argv[2],"w");
	fpin=fopen(argv[1],"r");
	getToken();
	strcpy(token,sym[symst++]);
	ret = CompUnit();
	if(ret!=0) return ret;
	return 0;
} 

//进制转换
void ChangeTen(int n, char str[]){       //将n进制数转换成10进制数
    int len=strlen(str),i,sum=0,t=1;
    for(i=len-1; i >= 0; i--){
        if(str[i]>='A'&&str[i]<'G'){     
            sum+=(str[i]-55)*t;
        }
        else if(str[i]>='a'&&str[i]<'g'){
        	sum+=(str[i]-'a'+10)*t;
		}
        else{
            sum+=(str[i] - 48)*t;
        }
        t*=n;
    }
    sprintf(sym[symed++],"Number(%d)",sum);
}
//词法分析 
int getToken(){
	int note=0;
	tst=0;
	while(fgets(str,250,fpin)!=NULL){
		memset(token,0,sizeof(token));
		int iskey=0;
		sst=0;
		while(sst<strlen(str)){
			memset(token,0,sizeof(token));
			iskey=0;
			char ch=str[sst];
			if(ch==' '){
				sst++;
			} 
			else if(ch=='/'&&str[sst+1]=='/'){
				break;
			}
			else if(note==1){
				if(ch=='*'&&str[sst+1]=='/'){
					sst+=2;
					note=0;
					continue;
				}
				sst++;
			}
			else if(ch=='/'&&str[sst+1]=='*'){
				note=1;
				sst++;
			}
			//Ident或者关键字 
			else if((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')){
				token[tst++]=ch;
				ch=str[++sst];
				while((ch>='0'&&ch<='9')||(ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')){
					token[tst++]=ch;
					ch=str[++sst];
				}
				token[tst]='\0';
				tst=0;
				for(int i=0;i<=2;i++){
					if(strcmp(token,key[i])==0){
						strcpy(sym[symed++],keyOut[i]);
						iskey=1;
					}
				}
			}
			//Number类 
			else if(ch>='0'&&ch<='9'){
				if(ch!='0'){
					token[tst++]=ch;
					ch=str[++sst];
					while(ch>='0'&&ch<='9'){
						token[tst++]=ch;
						ch=str[++sst];
					}
					token[tst]='\0';
					tst=0;
					ChangeTen(10,token);
				}
				else{
					//16进制 
					if(str[sst+1]=='x'||str[sst+1]=='X'){
						sst++;
						ch=str[++sst];
						if(ch>='0'&&ch<='9'){
							token[tst++]=ch;
							ch=str[++sst];
							while(ch>='0'&&ch<='9'){
								token[tst++]=ch;
								ch=str[++sst];
							}
							token[tst]='\0';
							tst=0;
							ChangeTen(16,token);
						}
						//16进制Number出错 
						else{
							return 16;
						} 
					}
					//8进制 
					else{
						if(str[sst+1]>='0'&&str[sst+1]<='9'){
							ch=str[++sst];
							token[tst++]=ch;
							ch=str[++sst];
							while(ch>='0'&&ch<='9'){
								token[tst++]=ch;
								ch=str[++sst];
							}
							token[tst]='\0';
							tst=0;
							ChangeTen(8,token);
						}
						else{
							if(str[sst+1]==' '){
								ChangeTen(10,"0");
							}
							else{
								return 8;
							}
						}
					}
				}
			}
			else if(ch=='='){
				if(str[++sst]=='='){
					fprintf(fpout,"Eq\n");sst++;
				}
				else{
					printf(fpout,"Assign\n");sst++;
					sst--;
				}
			}
			else if(ch==';'){
				strcpy(sym[symed++],"Semicolon");
				sst++;
			}
			else if(ch=='('){
				strcpy(sym[symed++],"LPar");
				sst++;
			}
			else if(ch==')'){
				strcpy(sym[symed++],"RPar");
				sst++;
			}
			else if(ch=='{'){
				strcpy(sym[symed++],"LBrace");
				sst++;
			}
			else if(ch=='}'){
				strcpy(sym[symed++],"RBrace");
				sst++;
			}
			else if(ch=='+'){
				printf("Plus\n");sst++;
			}
			else if(ch=='*'){
				printf("Mult\n");sst++;
			} 
			else if(ch=='/'){
				printf("Div\n");sst++;
			}
			else if(ch=='<'){
				printf("Lt\n");sst++;
			}
			else if(ch=='>'){
				printf("Gt\n");sst++;
			}
			else{
				printf("Err\n");return 0;
			}
			if(sst==strlen(str)){
				break;
			};
		}
	}
} 
int CompUnit(){
	ret = FuncDef();
	if(ret!=0) return ret;
	strcpy(token,sym[symst++]);
	if(token[0]!=0){
		printf("error in CompUnit");
	} 
	return 0;
}
int FuncDef(){
	ret = FuncType();
	if(ret!=0) return ret;
	strcpy(token,sym[symst++]);
	ret = Ident();
	if(ret!=0) return ret;
	strcpy(token,sym[symst++]);
	if(strcmp(token,"LPar")!=0){
		printf("error in FuncDef '('");
		return 103;
	}
	strcpy(token,sym[symst++]);
	if(strcmp(token,"RPar")!=0){
		printf("error in FuncDef ')'");
		return 104;
	}
	strcpy(token,sym[symst++]);
	ret = Block();
	if(ret!=0) return ret; 
	return 0;
}
int FuncType(){
	if(strcmp(token,"Int")!=0){
		printf("error in FuncType");
		return 101;
	}
	fprintf(fpout,"define dso_local i32 ");
	return 0;
} 
int Ident(){
	if(strcmp(token,"Main")!=0){
		printf("error in Ident");
		return 102;
	}
	fprintf(fpout,"@main()");
	return 0;
}
int Block(){
	if(strcmp(token,"LBrace")!=0){
		printf("error in Block '{'");
		return 105;
	}
	fprintf(fpout,"{\n");
	strcpy(token,sym[symst++]);
	ret = Stmt(); 
	if(ret!=0) return ret;
	strcpy(token,sym[symst++]);
	if(strcmp(token,"RBrace")!=0){
		printf("error in Block '}'");
		return 107;
	}
	fprintf(fpout,"}");
	return 0;
}
char tempNum[20];
int Stmt(){
	if(strcmp(token,"Return")!=0){
		printf("error in Stmt 'return'");
		//return 105;
	}
	fprintf(fpout,"    ret ");
	strcpy(token,sym[symst++]);
	if(token[0]=='N'&&token[1]=='u'&&token[4]=='e'&&token[5]=='r'){
		sscanf(token,"%*[^(](%[^)]",tempNum);
		fprintf(fpout,"i32 %s",tempNum);
		
	}
	else{
		printf("error in Stmt 'Number'");
		return 106;
	}
	strcpy(token,sym[symst++]);
	if(strcmp(token,"Semicolon")!=0){
		printf("error in Stmt ';'");
		return 105;
	}
	fprintf(fpout,"\n");
	return 0;
}
int getGrammar(){
	
}
