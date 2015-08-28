#include<bits/stdc++.h>
using namespace std;

#include"tseitin-common.cpp"

// this solution assumes that the top-left corner is a 1, all others vertices are 0.
void DECIDE(int l){
	cout << "d " << l << " ";
}

map<int,bool>seen;
void Solve(int l,int r){
	if(r==l+1) // brute force (l)--(l+1)
		for(int msk=0;msk<(1<<(h-1));msk++)
			for(int bit=(msk==0)?(h-2):(__builtin_ctz(msk)-1);bit>=0;bit--)
				DECIDE(-Eid[enc(l,bit)][enc(l+1,bit)]);
	else if(r==l+2) // brute force (l)--(l+1) and (l+1)--(l+2)
		for(int msk=0;msk<(1<<(h-1));msk++){
			for(int bit=(msk==0)?(h-2):(__builtin_ctz(msk)-1);bit>=0;bit--)
				DECIDE(-Eid[enc(l,bit)][enc(l+1,bit)]);
			for(int msk2=0;msk2<(1<<(h-1));msk2++)
				for(int bit=(msk2==0)?(h-2):(__builtin_ctz(msk2)-1);bit>=0;bit--)
					DECIDE(-Eid[enc(l+1,bit)][enc(l+2,bit)]);
		}
	else{
		int m = (l + r) / 2;
		// [l..m], [m+1..r].
		for(int msk=0;msk<(1<<h);msk++){
			for(int bit=(msk==0)?(h-1):(__builtin_ctz(msk)-1);bit>=0;bit--)
				DECIDE(-Eid[enc(m,bit)][enc(m+1,bit)]);
			if(__builtin_popcount(msk)%2==1)
				Solve(m+1,r);
			else
				Solve(l,m);
		}
	}
}

int main(){
	cin >> w >> h;
	assert(w > 1 && h > 1);
	genvarorder();
	Solve(0,w-1);
	cout << endl;
}
