#include<bits/stdc++.h>
using namespace std;

#include"tseitinDblEdges-common.cpp"

// generates grid s.t. the top-left corner is a 1, all others vertices are 0.
int main(){
	cin >> l >> w;
	genvarorder();
	vector<vector<int>> S(w,vector<int>(l,0));
	S[0][0]=1;
	int CL=0;
	for(int y=0;y<w;y++)for(int x=0;x<l;x++){
		CL+=1<<(2*(int)Eid1[enc(y,x)].size()-1);
	}
	printf("p cnf %d %d\n",nE,CL);
	for(int y=0;y<w;y++)for(int x=0;x<l;x++){
		vector<int> v;
		for(auto p:Eid1[enc(y,x)])v.push_back(p.second);
		for(auto p:Eid2[enc(y,x)])v.push_back(p.second);
		for(int msk=0;msk<(1<<((int)v.size()));msk++){
			if(__builtin_popcount(msk)%2 != S[y][x]){
				for(int i=0;i<(int)v.size();i++){
					if(msk&(1<<i))cout<<-v[i]<<" ";else cout<<v[i]<<" ";
				}
				cout<<"0"<<endl;
			}
		}
	}
}
