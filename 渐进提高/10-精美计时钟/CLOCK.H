#ifndef __CLOCK_H
#define __CLOCK_H

void Ini_Clock_Contex(void)                                  ;
void Gen_Hour_Hand(unsigned int angle)                       ; // ����ʱ��ͼ������
void Save_Hour_Hand_BK(void)                                 ; // ����ʱ���ڵ��ı���
void Draw_Hour_Hand(unsigned int color)                      ; // ��ʱ��ͼ������
void Restore_Hour_Hand_BK(void)                              ; // �ָ�ʱ���ڵ�����
void Gen_Minu_Hand(unsigned int angle)                       ; // ���ɷ���ͼ������
void Save_Minu_Hand_BK(void)                                 ; // ���ݷ����ڵ��ı���
void Draw_Minu_Hand(unsigned int color)                      ; // ��ʱ��ͼ������
void Restore_Minu_Hand_BK(void)                              ; // �ָ������ڵ�����
void Gen_Second_Hand(unsigned int angle)                     ; // ��������ͼ������
void Save_Second_Hand_BK(void)                               ; // ���������ڵ��ı���
void Draw_Second_Hand(unsigned int color)                    ; // ��ʱ����ͼ������
void Restore_Second_Hand_BK(void)                            ; // �ָ������ڵ�����

#endif