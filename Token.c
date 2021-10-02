#include<stdio.h>
#include<string.h>
#include<stdlib.h>
char str[256];
char token[15];
int sst=0;  //表示句子读的位置 
int tst=0;  //表示词读的位置 
char *key[7]={"if","else","while","break","continue","return"};
char *keyOut[7]={"If","Else","While","Break","Continue","Return"};
int main(){
	while(gets(str)!=NULL){
		int iskey=0;
		sst=0;
		while(sst<strlen(str)){
			iskey=0;
			char ch=str[sst];
			if(ch==' '){
				sst++;
			} 
			if(ch=='\\'&&str[sst+1]=='t'){
				sst+=2;
				continue;
			} 
			else if((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||(ch=='_')){
				token[tst++]=ch;
				ch=str[++sst];
				while((ch>='0'&&ch<='9')||(ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||(ch=='_')){
					token[tst++]=ch;
					ch=str[++sst];
				}
				token[tst]='\0';
				tst=0;
				for(int i=0;i<=5;i++){
					if(strcmp(token,key[i])==0){
						printf("%s\n",keyOut[i]);
						iskey=1;
					}
				}
				if(iskey==0) printf("Ident(%s)\n",token);
			}
			else if(ch>='0'&&ch<='9'){
				token[tst++]=ch;
				ch=str[++sst];
				while(ch>='0'&&ch<='9'){
					token[tst++]=ch;
					ch=str[++sst];
				}
				token[tst]='\0';
				tst=0;
				printf("Number(%s)\n",token);
			}
			else if(ch=='='){
				if(str[++sst]=='='){
					printf("Eq\n");sst++;
				}
				else{
					printf("Assign\n");sst++;
					sst--;
				}
			}
			else if(ch==';'){
				printf("Semicolon\n");sst++;
			}
			else if(ch=='('){
				printf("LPar\n");sst++;
			}
			else if(ch==')'){
				printf("RPar\n");sst++;
			}
			else if(ch=='{'){
				printf("LBrace\n");sst++;
			}
			else if(ch=='}'){
				printf("RBrace\n");sst++;
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
