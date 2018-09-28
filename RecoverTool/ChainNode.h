
//这个结构是链表的节点
typedef struct ChainNode
{
	int Index;//节点序号，用于识别每个结点，表头的该成员用于统计结点总数
	LONG EncryptStart;
	DWORD DataLength;
	struct ChainNode* next; //由于需要动态分配，所以只能在内存中先以链表形式保存，当最终存入文件时在计算链表所占用空间，然后分配空间并存入，最后写入到文件尾部。

} ChainNode;

//获得链表的节点总数
int GetChainNodeCount();

//创建链表表头
ChainNode* CreateChainHead();

//插入节点
void InsertNode(ChainNode* p);

//删除节点
void DeleteNode(int Index);

//搜索节点
ChainNode* SearchChainNode(int Index);

//释放链表节点
void FreeChain();