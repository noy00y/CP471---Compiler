def int gcd(int a, int b)
	if(a==b) then
		return (a) 
	fi;
	if(a>b) then
		a = (gcd(a-b,b))
	else 
		return(gcd(a,b-a, a)) 
	fi;
fed;
print gcd(21.15,15)