#include <iostream>
#include <string>
#include <vector>
#include <LzmaLib/lzma.h>

using namespace std;

template <typename TBuffer>
void test(TBuffer &buf)
{
	lzma::PackedData<TBuffer> pd;
	if (!pd.pack(buf))
	{
		cout << "Compress: Error\n";
		return;
	}
	cout << "Compress: OK\n";

	TBuffer unpackBuf;
	if (!pd.unpack(unpackBuf))
	{
		cout << "Decompress: Error\n";
		return;
	}
	cout << "Decompress: OK\n";
	if (buf == unpackBuf)
		cout << "Decompress: Data equal!\n";		
}

int main()
{
	string s("1_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_2");
	test(s);
	vector<char> v(s.begin(), s.end());
	test(v);
	vector<unsigned char> v2(s.begin(), s.end());
	test(v2);
}