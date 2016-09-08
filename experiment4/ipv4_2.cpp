/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysInclude.h"

/*���ö����������Ϊ·�ɱ����*/
typedef struct stu_table_record{
	unsigned int dest;		//Ŀ���ַ
	unsigned int masklen;	//���볤��
	unsigned int nexthop;		//��һ����ַ
	struct stu_table_record *lrecord;	//������(С)
	struct stu_table_record *rrecord;	//������(��)
}stu_table_record;

// system support
extern void fwd_LocalRcv(char *pBuffer, int length);

extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);

extern void fwd_DiscardPkt(char *pBuffer, int type);

extern unsigned int getIpv4Address( );

stu_table_record *stu_fwardtable = NULL;    // �������ĸ�
// implemented by students

void stud_Route_Init(){
	if((stu_fwardtable = (stu_table_record*)malloc(sizeof(stu_table_record))))
	{
		stu_fwardtable->dest = 0;
		stu_fwardtable->masklen = 0;
		stu_fwardtable->nexthop = 0;
		stu_fwardtable->lrecord = NULL;
		stu_fwardtable->rrecord = NULL;
	}
	else
	{
		printf("Init failed.\n");
		exit(0);
	}
	return;
}

void stud_route_add(stud_route_msg *proute){
	stu_table_record *tmp = stu_fwardtable;
	unsigned int masklen=ntohl(proute->masklen);
	unsigned int dest=ntohl(proute->dest);
	unsigned int record_dest=dest>>(32-masklen)<<(32-masklen);    // ��¼Ŀ���ַ�������ַ,��λʹ��λ��0
	unsigned int nexthop=proute->nexthop;
	
	if(tmp->dest == 0 && tmp->masklen == 0 && tmp->nexthop == 0)    // û���κμ�¼
	{
		tmp->dest = record_dest;
		tmp->masklen = masklen;
		tmp->nexthop = nexthop;
	}
	else
	{
		stu_table_record *last = tmp;    // ��һ����㣬�½ڵ��������/��
		int to_left = 1;    // ����:1,����:0.
		
		while(tmp)
		{
			if(tmp->dest < record_dest)    // �½�����,������������
			{
				last = tmp;
				to_left = 0;
				tmp = tmp->rrecord;
			}
			else if(tmp->dest > record_dest)    // �½���С,������������
			{
				last = tmp;
				to_left = 1;
				tmp = tmp->lrecord;
			}
			else    // �ҵ����½����ͬ�ģ�������� 
			{
				return;
			}
		}
		
		if((tmp = (stu_table_record*)malloc(sizeof(stu_table_record))))
		{
			// ��������Ϣ
			tmp->dest = record_dest;
			tmp->masklen = masklen;
			tmp->nexthop = nexthop;
			tmp->lrecord = NULL;
			tmp->rrecord = NULL;
			// ���뵽��Ӧλ��
			if(to_left)
			{
				last->lrecord = tmp;
			}
			else
			{
				last->rrecord = tmp;
			}
		}
		else
		{
			printf("Update failed.\n");
			exit(0);
		}
	}
	return;
}

SHORT calChecksum(USHORT *buffer,int len){    // ����У��� 
	int sum = 0;    // �ĸ��ֽ������ɽ�λ
	// ��ֹ�����ֽ������ֽ���Ӻ󣬽�βʣһ���ֽڳ���
	while(len > 1){
		sum += *buffer ++;    // 16λ�������ֽ������ֽ����
		len -= sizeof(USHORT);
	}
	// ��ʣ�µ�һ���ֽ�
	if(len){
		sum += *(UCHAR *)buffer;
	}
	// �ؾ�
	while(sum>>16){
		sum = sum>>16 + sum<<16>>16;
	}
	return (USHORT)(~sum);    // ȡ��
}

unsigned int get_nexthop(unsigned int dest){//������һ��
	stu_table_record *tmp = stu_fwardtable;
	unsigned int nexthop = 0;

	while(tmp){    // �Ƚ�Ŀ���ַ��·�ɱ��еĵ�ַ֮��Ĺ�ϵ
		if(tmp->dest == dest>>(32-tmp->masklen)<<(32-tmp->masklen)){    // �ҵ���Ӧ��·����
			nexthop = tmp->nexthop;
			tmp = tmp->rrecord;    // Ѱ���ƥ��
		}
		else if(tmp->dest < dest>>(32-tmp->masklen)<<(32-tmp->masklen)){
			tmp = tmp->lrecord;
		}
		else{
			tmp = tmp->rrecord;
		}
	}
	return nexthop;
}

int stud_fwd_deal(char *pBuffer, int length){
	// �ж�TTL�Ƿ�Ϊ0�����ǣ�����
	if(pBuffer[8] == 0x00){    // ttlռһ���ֽڣ�������ֱ��ȡ��
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR);	
		return 1;
	}

	// �Ƿ񷢸�����
	unsigned int address = getIpv4Address();    // ����ipv4��ַ
	char *tempAddress = pBuffer + 16;    // ipv4����ͷ�У�Ŀ�ĵ�ַ��ռ4���ֽڣ�����ʼλ��
	unsigned int *intAddress = (unsigned int *)tempAddress;    // ȡ4���ֽ�
	unsigned int dest = ntohl(*intAddress);
	if(address == dest){
		fwd_LocalRcv(pBuffer, length);
		return 0;
	}
	
	unsigned int nexthop = get_nexthop(dest);
	if(nexthop == 0){    // û�б���
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE);
		return 1;
	}

	pBuffer[8] --;    // TTL - 1

	// ���¼���У���
	char *tempchecksum = pBuffer + 10;    // ipv4����ͷ�У�checksum��ռ2���ֽڣ�����ʼλ��
	short int *checksum = (short int *)tempchecksum;    // ȡ2���ֽ�
	*checksum = 0;    // ������
	*checksum = calChecksum((USHORT *)pBuffer, 20);    // ����У���

	/*ת��*/
	fwd_SendtoLower(pBuffer, length, nexthop);
	return 0;
}
