
//����ṹ������Ľڵ�
typedef struct ChainNode
{
	int Index;//�ڵ���ţ�����ʶ��ÿ����㣬��ͷ�ĸó�Ա����ͳ�ƽ������
	LONG EncryptStart;
	DWORD DataLength;
	struct ChainNode* next; //������Ҫ��̬���䣬����ֻ�����ڴ�������������ʽ���棬�����մ����ļ�ʱ�ڼ���������ռ�ÿռ䣬Ȼ�����ռ䲢���룬���д�뵽�ļ�β����

} ChainNode;

//�������Ľڵ�����
int GetChainNodeCount();

//���������ͷ
ChainNode* CreateChainHead();

//����ڵ�
void InsertNode(ChainNode* p);

//ɾ���ڵ�
void DeleteNode(int Index);

//�����ڵ�
ChainNode* SearchChainNode(int Index);

//�ͷ�����ڵ�
void FreeChain();