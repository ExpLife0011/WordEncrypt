// ChainNode.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h> 
#include<iostream>
using namespace std;
#include "ChainNode.h"

//表头
ChainNode* head = NULL;

//获得链表的节点总数
int GetChainNodeCount()
{
	if (head == NULL)
	{		
		return 0;

	}else
	{
		return head->Index;
	}
}

//创建链表表头
ChainNode* CreateChainHead()// 创建链表头
{
	head = (ChainNode*)malloc(sizeof(ChainNode));// 分配表头节点的空间
	memset(head,0,sizeof(ChainNode));
	return (head);

}

//插入节点
void InsertNode(ChainNode* p)//在链表的最后插入节点
{

	ChainNode* q;

	if (head == NULL)
	{
		head = CreateChainHead();
	}
		
	p->Index = head->Index;
	head->Index += 1;//把链表的总节点数加1
	q = head;//若表头的next为NULL，则在表头之后插入结点

	while( q->next != NULL)
	{
		q= q->next;
	}//找到最后一个节点的地址存于q中

	q->next=p;	

}

//删除节点
void DeleteNode(int Index)
{
	ChainNode* p,*q,*temp;
	if (GetChainNodeCount()==0)
	{
		return;
	}		
	q = head;
	p = q->next;
	while(( p != NULL) && ( p->Index != Index))
	{
		q = p;// q 用于记住上一个节点的位置
		p = p->next;
	}

	if( p == NULL )
		return;//没有找到要删除的节点
	else
	{	
		q->next = p->next;//上一个的下一个设置为要删除节点的下一个节点
		temp = p->next;//temp设置为要删除节点的下一个节点，用于后面索引重置工作
		free(p);//到这里就完成了删除节点&释放节点所占空间的工作，下面要调整被删节点的后继的所有节点的序号
		p = NULL;//防止野指针
		head->Index -= 1;//删除后再将表头结点减1，不能先将链表节点长度减1
		while( temp != NULL)//把已经删除节点的下一个节点开始做索引重置工作
		{
			temp->Index -= 1;//从要删除节点的下一个节点开始调整索引号
			temp = temp->next;
		}
	}

}

//搜索节点
ChainNode* SearchChainNode(int Index)//搜索群组的节点
{
	ChainNode* p;
	if (GetChainNodeCount()==0)
	{
		return NULL;
	}
	p = head->next;
	while(( p != NULL) && ( p->Index != Index))
	{
		p = p->next;
	}
	if( p == NULL )
		return NULL;
	else
		return p;
}

//释放链表节点
void FreeChain()
{	
	ChainNode* p,*q;
	if (GetChainNodeCount()==0)
	{
		return;
	}		
	p = head;//从链表的第一个节点，即表头结点开始释放
	while(p != NULL)
	{
		q = p->next;//用q记住要释放节点的下一个节点的地址
		free(p);//在这里释放群组节点
		p = NULL;//防止野指针
		p = q;
	}
}

void Enumeration()
{
	ChainNode* p;
	if (GetChainNodeCount() == 0)
	{
		return;
	}
	p = head->next;
	while(p!=NULL)
	{
//		 cout<<p->Data<<endl;
		 p = p->next;
	}
}

/* Call Example For ChainNode

	ChainNode* Example = NULL;
	Example = (ChainNode*)malloc(sizeof(ChainNode));// 分配表头节点的空间
	memset(Example,0,sizeof(ChainNode));

	Example->Data = 8;

	InsertNode(Example);

	Enumeration();

*/
