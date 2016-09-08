/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"

extern void ip_DiscardPkt(char* pBuffer,int type);

extern void ip_SendtoLower(char*pBuffer,int length);

extern void ip_SendtoUp(char *pBuffer,int length);

extern unsigned int getIpv4Address();

// implemented by students

SHORT calChecksum(USHORT *buffer,int len){    // ����У��� 
	unsigned long sum = 0;    // �ĸ��ֽ������ɽ�λ
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
	while (sum>>16){
		sum = (sum>>16) + (sum & 0xffff);
	}
	return (USHORT)(~sum);    // ȡ��
}

int stud_ip_recv(char *pBuffer,unsigned short length)
{
	// char ռһ���ֽڣ�һ������ȡ��8λ��version �� ihl һ��ȡ������Ҫ��λ����������Ҫ��
	// �ж�version �Ƿ���ȷ��4
	if((pBuffer[0] & 0xf0) != 0x40){    // ȡ����λ
		ip_DiscardPkt(pBuffer,STUD_IP_TEST_VERSION_ERROR);
		return 1;
	}
	// �ж�IHL�Ƿ���ȷ��5
	if((pBuffer[0] & 0x0f) != 0x05){    // ȡ����λ
		ip_DiscardPkt(pBuffer,STUD_IP_TEST_HEADLEN_ERROR);
		return 1;
	}
	// �ж�TTL�Ƿ�Ϊ0�����ǣ�����
	if(pBuffer[8] == 0x00){    // ttlռһ���ֽڣ�������ֱ��ȡ��
		ip_DiscardPkt(pBuffer,STUD_IP_TEST_TTL_ERROR);
		return 1;
	}
	// �ж�ͷУ����Ƿ���ȷ��Ϊ0
	SHORT sum = calChecksum((USHORT *)pBuffer,length);
	if(sum != 0){
		ip_DiscardPkt(pBuffer,STUD_IP_TEST_CHECKSUM_ERROR);
		return 1;
	}
	// Ŀ�ĵ�ַ�Ƿ���ȷ���뱾����ַ��ͬ
	unsigned int address = getIpv4Address();    // ����ipv4��ַ
	char *tempAddress = pBuffer + 16;    // ipv4����ͷ�У�Ŀ�ĵ�ַ��ռ4���ֽڣ�����ʼλ��
	unsigned int *intAddress = (unsigned int *)tempAddress;    // ȡ4���ֽ�
	if(address != ntohl(*intAddress)){
		ip_DiscardPkt(pBuffer,STUD_IP_TEST_DESTINATION_ERROR);
		return 1;
	}
	ip_SendtoUp(pBuffer,length);    // ����
	return 0;
}

int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,
				   unsigned int dstAddr,byte protocol,byte ttl)
{
	byte *datagram = new byte[20 + len];    // ����ռ䣬����ͷ�� 20�ֽ�
	datagram[0] = 0x45;    // version=4��IHL=5��
	datagram[1] = 0x80;    // Type of service = 0x800;
	// �ܳ���
	byte *dag_hl = datagram + 2;    // �ܳ�����ʼλ��
	unsigned short int *length = (unsigned short int *)dag_hl;    // �ܳ���ռ�����ֽڣ�����
	*length = htons(20 + len);
	// ��ʶ�������ֽ�
	datagram[4] = 0x00;
	datagram[5] = 0x00;
	// ��־λ��Ƭƫ�ƣ������ֽ�
	datagram[6] = 0x00;
	datagram[7] = 0x00;

	datagram[8] = ttl;
	datagram[9] = protocol;
	// �ײ�У��ͣ������ֽڣ�����ǰ����
	datagram[10] = 0x00;
	datagram[11] = 0x00;
	// Դ��ַ
	byte *dag_srcAddr = datagram + 12;    // Դ��ַ��ʼλ��
	unsigned int *srcAddrTemp = (unsigned int *)dag_srcAddr;    // Դ��ַռ�ĸ��ֽڣ�����
	*srcAddrTemp = ntohl(srcAddr);
	// Ŀ�ĵ�ַ
	byte *dag_dstAddr = datagram + 16;    // Ŀ�ĵ�ַ��ʼλ��
	unsigned int *dstAddrTemp = (unsigned int *)dag_dstAddr;    // Ŀ�ĵ�ַռ�ĸ��ֽڣ�����
	*dstAddrTemp = ntohl(dstAddr);
	// �������ײ�У���
	byte *dag_checksum = datagram + 10;    // У�����ʼλ��
	short int *headerChecksum = (short int *)dag_checksum;    // У���ռ�ĸ��ֽڣ�����
	*headerChecksum = calChecksum((USHORT *)datagram,20);
	// ��װ�ϲ㴫���� segment
	for(int i = 0; i < len; i ++){
		 datagram[i + 20] = pBuffer[i];
	}
	// ���ݸ��²�
	ip_SendtoLower(datagram,20 + len);
	return 0;
}
