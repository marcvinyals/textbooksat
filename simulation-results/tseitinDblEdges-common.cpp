map<int,map<int,int> > Eid1,Eid2;
int l,w;
int enc(int y,int x){return l*y+x;}
int nE=0;

// NOTE: do not change variable order
// (because forget-plugin hack in textbooksat depends on it)
void genvarorder(){
	for(int dir=0;dir<2;dir++)for(int x=0;x<l;x++)for(int y=0;y<w;y++){
		int y2=y+dir, x2=x+1-dir;
		if(0<=y2 && y2<w && 0<=x2 && x2<l){
			Eid1[enc(y,x)][enc(y2,x2)]=++nE;
			Eid1[enc(y2,x2)][enc(y,x)]=nE;
			Eid2[enc(y,x)][enc(y2,x2)]=++nE;
			Eid2[enc(y2,x2)][enc(y,x)]=nE;
		}
	}
}
