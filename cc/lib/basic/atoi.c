/*
 *  atoi.c
 */

int atoi( s )
const char *s;
{
    char c;
    int i = 0, pm = 1, r = 10;
    
    c = *s++;
    if (c == '-') {
	pm = -1;  c = *s++;
    }
    if (c == '0') {
	r = 8;  c = *s++;
    }
    if (c == 'x') {
	r = 16;  c = *s++;
    }

    while (c) {
	if (c >= '0' && c <= '9' && c - '0' < r) {
	    i = i*r + (c - '0');  c = *s++;
	}
	else if (c >= 'a' && c <= 'f' && r == 16) {
	    i = i*r + (c - 'a' + 10);  c = *s++;
	}
	else if (c >= 'A' && c <= 'F' && r == 16) {
	    i = i*r + (c - 'A' + 10);  c = *s++;
	}
	else c = 0;
    }
    return i * pm;
}

