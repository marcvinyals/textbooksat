#include<bits/stdc++.h>
using namespace std;

#include"tseitin-common.cpp"

// this solution assumes that the top-left corner is a 1, all others vertices are 0.
void DECIDE(int l){
	cout << "d " << l << " ";
}

map<int,bool>seen;
void Solve(int l){
	if(l==w-2){
		for(int msk=0;msk<(1<<(h-1));msk++)
			for(int bit=(msk==0)?(h-2):(__builtin_ctz(msk)-1);bit>=0;bit--)
				DECIDE(-Eid[enc(l,bit)][enc(l+1,bit)]);
	}else{
		if(!seen[l]){
			seen[l] = true;
			for(int msk=0;msk<(1<<h);msk++){
				for(int bit=(msk==0)?(h-1):(__builtin_ctz(msk)-1);bit>=0;bit--)
					DECIDE(-Eid[enc(l+1,bit)][enc(l+2,bit)] * ((msk%2==0 && __builtin_popcount(msk)%2==0 && bit==0)?-1:1));
				if(msk%2==0)
					Solve(l+2);
				else
					for(int msk=0;msk<(1<<(h-1));msk++)
						for(int bit=(msk==0)?(h-2):(__builtin_ctz(msk)-1);bit>=0;bit--)
							DECIDE(-Eid[enc(l,bit)][enc(l+1,bit)]);
			}
		}else{
			for(int msk=0;msk<(1<<(h-1));msk++){
				for(int bit=(msk==0)?(h-2):(__builtin_ctz(msk)-1);bit>=0;bit--)
					DECIDE(-Eid[enc(l,bit)][enc(l+1,bit)]);
				for(int msk2=0;msk2<(1<<(h-1));msk2++)
					for(int bit=(msk2==0)?(h-2):(__builtin_ctz(msk2)-1);bit>=0;bit--)
						DECIDE(-Eid[enc(l+1,bit)][enc(l+2,bit)]);
			}
		}
	}
}

int main(){
	cin >> w >> h;
	assert(w > 1 && h > 1);
	assert(w % 2 == 0);
	genvarorder();
	Solve(0);
	cout << endl;
}
