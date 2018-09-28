// ChainNode.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h> 
#include<iostream>
using namespace std;
#include "ChainNode.h"

//��ͷ
ChainNode* head = NULL;

//�������Ľڵ�����
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

//���������ͷ
ChainNode* CreateChainHead()// ��������ͷ
{
	head = (ChainNode*)malloc(sizeof(ChainNode));// �����ͷ�ڵ�Ŀռ�
	memset(head,0,sizeof(ChainNode));
	return (head);

}

//����ڵ�
void InsertNode(ChainNode* p)//�������������ڵ�
{

	ChainNode* q;

	if (head == NULL)
	{
		head = CreateChainHead();
	}
		
	p->Index = head->Index;
	head->Index += 1;//��������ܽڵ�����1
	q = head;//����ͷ��nextΪNULL�����ڱ�ͷ֮�������

	while( q->next != NULL)
	{
		q= q->next;
	}//�ҵ����һ���ڵ�ĵ�ַ����q��

	q->next=p;	

}

//ɾ���ڵ�
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
		q = p;// q ���ڼ�ס��һ���ڵ��λ��
		p = p->next;
	}

	if( p == NULL )
		return;//û���ҵ�Ҫɾ���Ľڵ�
	else
	{	
		q->next = p->next;//��һ������һ������ΪҪɾ���ڵ����һ���ڵ�
		temp = p->next;//temp����ΪҪɾ���ڵ����һ���ڵ㣬���ں����������ù���
		free(p);//������������ɾ���ڵ�&�ͷŽڵ���ռ�ռ�Ĺ���������Ҫ������ɾ�ڵ�ĺ�̵����нڵ�����
		p = NULL;//��ֹҰָ��
		head->Index -= 1;//ɾ�����ٽ���ͷ����1�������Ƚ�����ڵ㳤�ȼ�1
		while( temp != NULL)//���Ѿ�ɾ���ڵ����һ���ڵ㿪ʼ���������ù���
		{
			temp->Index -= 1;//��Ҫɾ���ڵ����һ���ڵ㿪ʼ����������
			temp = temp->next;
		}
	}

}

//�����ڵ�
ChainNode* SearchChainNode(int Index)//����Ⱥ��Ľڵ�
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

//�ͷ�����ڵ�
void FreeChain()
{	
	ChainNode* p,*q;
	if (GetChainNodeCount()==0)
	{
		return;
	}		
	p = head;//������ĵ�һ���ڵ㣬����ͷ��㿪ʼ�ͷ�
	while(p != NULL)
	{
		q = p->next;//��q��סҪ�ͷŽڵ����һ���ڵ�ĵ�ַ
		free(p);//�������ͷ�Ⱥ��ڵ�
		p = NULL;//��ֹҰָ��
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
	Example = (ChainNode*)malloc(sizeof(ChainNode));// �����ͷ�ڵ�Ŀռ�
	memset(Example,0,sizeof(ChainNode));

	Example->Data = 8;

	InsertNode(Example);

	Enumeration();

*/
