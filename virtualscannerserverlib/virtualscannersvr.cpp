
#include "virtualscannersvr.h"
#include <thread>
#include <vector>
#include "virtualscannersvrmain.h"

namespace DROWNINGLIU
{

	namespace VSCANNERSVRLIB
	{
		//全局变量
		char 			g_DirPath[512] = { 0 };						//上传文件路径
		unsigned int 	g_recvSerial = 0;							//服务器 接收的 数据包流水号
		unsigned int 	g_sendSerial = 0;							//服务器 发送的 数据包流水号
		long long int 	g_fileTemplateID = 562, g_fileTableID = 365;//服务器资源文件ID 
		char 			*g_downLoadDir = NULL;						//服务器资源路径
		char 			*g_fileNameTable = "downLoadTemplateNewestFromServer_17_05_08.pdf";	//数据库表名称
		char 			*g_fileNameTemplate = "C_16_01_04_10_16_10_030_B_L.jpg";			//模板名称
		int  			g_listen_fd = 0;							//监听套接字
	//	RecvAndSendthid_Sockfd		g_thid_sockfd_block[NUMBER];	//客户端信息
		char 			*g_sendFileContent = NULL;					//读取每轮下载模板的数据





		//对输入的数据进行解转义,  该函数中都是主调函数分配内存
		int  anti_escape(const char *inData, int inDataLenth, char *outData, int *outDataLenth)
		{
			int  ret = 0, i = 0;

			//socket_log( SocketLevel[2], ret, "func anti_escape() begin");
			if (NULL == inData || inDataLenth <= 0)
			{
				//	socket_log(SocketLevel[4], ret, "Error: inData : %p, || inDataLenth : %d", inData, inDataLenth);
				ret = -1;
				goto End;
			}


			const char *tmpInData = inData;
			const char *tmp = ++inData;
			char *tmpOutData = outData;
			int  lenth = 0;

			for (i = 0; i < inDataLenth; i++)
			{
				if (*tmpInData == 0x7d && *tmp == 0x01)
				{
					*tmpOutData = 0x7d;
					++tmpInData;
					++tmp;
					lenth += 1;
					i++;
				}
				else if (*tmpInData == 0x7d && *tmp == 0x02)
				{
					*tmpOutData = 0x7e;
					++tmpInData;
					++tmp;
					lenth += 1;
					i++;
				}
				else
				{
					*tmpOutData = *tmpInData;
					lenth += 1;
				}
				++tmpOutData;
				++tmpInData;
				++tmp;

			}



			*outDataLenth = lenth;
		End:
			//socket_log( SocketLevel[2], ret, "func anti_escape() end");
			return ret;
		}

		//对输入的数据进行转义
		int  escape(char sign, const char *inData, int inDataLenth, char *outData, int *outDataLenth)
		{
			int  ret = 0, i = 0;
			//socket_log( SocketLevel[2], ret, "func escape() begin");
			if (NULL == inData || inDataLenth <= 0)
			{
				//socket_log(SocketLevel[4], ret, "Error: NULL == inData || inDataLenth <= 0");
				ret = -1;
				goto End;
			}

			//printf("log------------------------1------------------\r\n");
			const char *tmpInData = inData;
			char *tmpOutData = outData;
			int  lenth = 0;

			for (i = 0; i < inDataLenth; i++)
			{
				if (*tmpInData == sign)
				{
					*tmpOutData = 0x7d;
					tmpOutData += 1;
					*tmpOutData = 0x02;
					lenth += 2;
				}
				else if (*tmpInData == 0x7d)
				{
					*tmpOutData = 0x7d;
					tmpOutData += 1;
					*tmpOutData = 0x01;
					lenth += 2;
				}
				else
				{
					*tmpOutData = *tmpInData;
					lenth += 1;
				}
				tmpOutData += 1;
				tmpInData++;

			}

			*outDataLenth = lenth;

		End:
			//socket_log( SocketLevel[2], ret, "func escape() end");
			return ret;
		}

		//散列算法 : crc32 校验
		static const int crc32tab[] = {
			0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
			0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
			0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
			0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
			0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
			0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
			0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
			0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
			0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
			0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
			0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
			0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
			0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
			0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
			0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
			0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
			0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
			0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
			0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
			0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
			0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
			0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
			0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
			0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
			0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
			0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
			0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
			0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
			0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
			0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
			0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
			0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
			0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
			0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
			0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
			0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
			0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
			0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
			0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
			0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
			0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
			0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
			0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
			0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
			0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
			0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
			0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
			0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
			0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
			0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
			0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
			0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
			0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
			0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
			0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
			0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
			0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
			0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
			0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
			0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
			0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L,
			0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
			0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
			0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
		};

		/*crc32校验
		*@param : buf  数据内容
		*@param : size 数据长度
		*@retval: 返回生成的校验码
		*/
		int crc326(const  char *buf, uint32_t size)
		{
			int i, crc;
			crc = 0xFFFFFFFF;
			for (i = 0; i < size; i++)
				crc = crc32tab[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
			return crc ^ 0xFFFFFFFF;
		}

		void assign_comPack_head(resCommonHead_t *head, int cmd, int contentLenth, int isSubPack, int isFailPack, int failPackIndex, int isRespond)
		{
			memset(head, 0, sizeof(resCommonHead_t));
			head->cmd = cmd;
			head->contentLenth = contentLenth;
			head->isSubPack = isSubPack;
			head->isFailPack = isFailPack;
			head->failPackIndex = failPackIndex;
			head->isRespondClient = isRespond;
			head->seriaNumber = g_sendSerial++;
		}

		//退出登录应答 数据包
		//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
		int VirtualScannerSession::exit_func_reply(char *recvBuf, int recvLen, int index)
		{
			int  ret = 0, outDataLenth = 0;					//发送数据的长度
			int  checkNum = 0;								//校验码
			char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
			char *sendBufStr = data_.data();						//发送数据地址
			char *tmp = NULL;
			resCommonHead_t head;							//应答数据报头

			//1. 打印数据包信息		
			//printf_comPack_news(recvBuf);			//打印接收图片的信息

													//2. 申请内存
			/*if ((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
			{
				ret = -1;
				myprint("Err : func mem_pool_alloc()");
				goto End;
			}*/

			//3. 数据信息拷贝
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			*tmp++ = 0x01;
			outDataLenth = sizeof(resCommonHead_t) + 1;

			//4. 组装报头
			assign_comPack_head(&head, LOGOUTCMDRESPON, outDataLenth, 0, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));

			//5. 计算校验码
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//6. 转义数据内容
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//7. 组合发送的内容
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			nLen2Write_ = outDataLenth + 2;
			/*if ((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, LOGOUTCMDRESPON)) < 0)
			{
				myprint("Err : func push_queue_send_block()");
				goto End;
			}

			sem_post(&(g_thid_sockfd_block[index].sem_send));*/

		End:
			//if (ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);

			return ret;
		}

		//登录应答数据包
		//recvBuf : 转义之后的数据:  包含 : 消息头 + 消息体 + 校验码     (只是去除了, 两个表示符);  recvLen : 长度为 buf 的内容 长度
		int VirtualScannerSession::login_func_reply(char *recvBuf, int recvLen, int index)
		{
			int  ret = 0, outDataLenth = 0;					//发送数据的长度
			int  checkNum = 0;								//校验码
			char tmpSendBuf[1500] = { 0 };					//临时数据缓冲区
			//char sendBufStr[2800] = { 0 };						//发送数据地址
			char *sendBufStr = data_.data();
			char *tmp = NULL;
			resCommonHead_t head;							//应答数据报头
			reqPackHead_t *tmpHead = NULL;					//请求信息
			char *userName = "WuKong";						//返回账户的用户名

															//1.打印接收图片的信息
			//printf_comPack_news(recvBuf);
			tmpHead = (reqPackHead_t *)recvBuf;
			g_recvSerial = tmpHead->seriaNumber;
			g_recvSerial++;

			////2.申请内存
			//if ((sendBufStr = mem_pool_alloc(g_memoryPool)) == NULL)
			//{
			//	ret = -1;
			//	myprint("Err : func mem_pool_alloc()");
			//	goto End;
			//}

			//3.组装数据包信息, 获取发送数据包长度
			tmp = tmpSendBuf + sizeof(resCommonHead_t);
			*tmp++ = 0x01;				//登录成功
			*tmp++ = 0x00;				//成功后, 此处信息不会进行解析, 随便写
			*tmp++ = strlen(userName);	//用户名长度
			memcpy(tmp, userName, strlen(userName));	//拷贝用户名	
			outDataLenth = sizeof(resCommonHead_t) + sizeof(char) * 3 + strlen(userName);

			//4.组装报头信息		
			assign_comPack_head(&head, LOGINCMDRESPON, outDataLenth, 0, 0, 0, 1);
			memcpy(tmpSendBuf, &head, sizeof(resCommonHead_t));
			//myprint("outDataLenth : %d, head.contentLenth : %d", outDataLenth, head.contentLenth);

			//5.组合校验码
			checkNum = crc326((const char *)tmpSendBuf, head.contentLenth);
			tmp = tmpSendBuf + head.contentLenth;
			memcpy(tmp, (char *)&checkNum, sizeof(checkNum));

			//6. 转义数据内容
			if ((ret = escape(0x7e, tmpSendBuf, head.contentLenth + sizeof(int), sendBufStr + 1, &outDataLenth)) < 0)
			{
				printf("Error : func escape() escape() !!!!; [%d], [%s]\n", __LINE__, __FILE__);
				goto End;
			}

			//7. 组合发送的内容
			*sendBufStr = 0x7e;
			*(sendBufStr + 1 + outDataLenth) = 0x7e;

			nLen2Write_ = outDataLenth + 2;
			//if ((ret = push_queue_send_block(g_sendData_addrAndLenth_queue, sendBufStr, outDataLenth + 2, LOGINCMDRESPON)) < 0)
			{
				//myprint("Err : func push_queue_send_block()");
				goto End;
			}

			//sem_post(&(g_thid_sockfd_block[index].sem_send));

		End:
			//if (ret < 0 && sendBufStr)   	mem_pool_free(g_memoryPool, sendBufStr);

			return ret;
		}

		//接收客户端的数据包, recvBuf: 包含: 标识符 + 报头 + 报体 + 校验码 + 标识符;   recvLen : recvBuf数据的长度
		//				sendBuf : 处理数据包, 将要发送的数据;  sendLen : 发送数据的长度
		int VirtualScannerSession::read_data_proce(char *recvBuf, int recvLen, int index)
		{
			int ret = 0;
			uint8_t  cmd = 0;

			//1.获取命令字
			cmd = *((uint8_t *)recvBuf);

			//2.选择处理方式
			switch (cmd) {
			case LOGINCMDREQ: 		//登录数据包
				if ((ret = login_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func login_func_reply() ");
				break;
			case LOGOUTCMDREQ: 		//退出数据包
				if ((ret = exit_func_reply(recvBuf, recvLen, index)) < 0)
					return -1;//myprint("Error: func exit_func_reply() ");
				break;
			//case HEARTCMDREQ: 		//心跳数据包
			//	if ((ret = data_un_packge(heart_beat_func_reply, recvBuf, recvLen, index)) < 0)
			//		myprint("Error: func heart_beat_func_reply() ");
			//	break;
			//case DOWNFILEREQ: 		//模板下载			
			//	if ((ret = data_un_packge(downLoad_template_func_reply, recvBuf, recvLen, index)) < 0)
			//		myprint("Error: func downLoad_template_func_reply() ");
			//	break;
			//case NEWESCMDREQ: 		//客户端信息请求
			//	if ((ret = data_un_packge(get_FileNewestID_reply, recvBuf, recvLen, index)) < 0)
			//		myprint("Error: func client_request_func_reply() ");
			//	break;
			//case UPLOADCMDREQ: 		//上传图片数据包
			//	if ((ret = data_un_packge(upload_func_reply, recvBuf, recvLen, index)) < 0)
			//		myprint("Error: func upload_func_reply() ");
			//	break;
			//case PUSHINFOREQ:		//消息推送
			//	if ((ret = data_un_packge(push_info_toUi_reply, recvBuf, recvLen, index)) < 0)
			//		myprint("Error: func push_info_toUi_reply() ");
			//	break;
			//case DELETECMDREQ: 		//删除图片数据包
			//	if ((ret = data_un_packge(delete_func_reply, recvBuf, recvLen, index)) < 0)
			//		myprint("Error: func delete_func_reply() ");
			//	break;
			//case TEMPLATECMDREQ:		//上传模板
			//	if ((ret = data_un_packge(template_extend_element_reply, recvBuf, recvLen, index)) < 0)
			//		myprint("Error: func template_extend_element() ");
			//	break;
			//case MUTIUPLOADCMDREQ:		//上传图片集
			//	if ((ret = data_un_packge(upload_template_set_reply, recvBuf, recvLen, index)) < 0)
			//		myprint("Error: func upload_template_set ");
			//	break;
#if 0	
			case ENCRYCMDREQ:
				if ((ret = data_un_packge(encryp_or_cancelEncrypt_transfer, recvBuf, recvLen, index)) < 0)
					myprint("Error: func encryp_or_cancelEncrypt_transfer_reply ");
				break;
#endif			
			default:
				//myprint("recvCmd : %d", cmd);
				assert(0);
			}


			return ret;
		}

		//function: find  the whole packet; retval maybe error
		int	VirtualScannerSession::find_whole_package(const char *recvBuf, int recvLenth, int index)
		{
			int ret = 0, i = 0;
			int  tmpContentLenth = 0;			//转义后的数据长度
			char tmpContent[1400] = { 0 }; 		//转义后的数据
			int flag = 0;				//flag 标识找到标识符的数量;
			int lenth = 0; 				//lenth : 缓存的数据包内容长度
			char content[2800] = { 0 };	//缓存完整包数据内容, 		
			char *tmp = NULL;


			tmp = content + lenth;
			for (i = 0; i < recvLenth; i++)
			{
				//1. 查找数据报头
				if (recvBuf[i] == 0x7e && flag == 0)
				{
					flag = 1;
					continue;
				}
				else if (recvBuf[i] == 0x7e && flag == 1)
				{
					//return 1;
					
					//2. 查找数据报尾, 获取到完整数据包, 将数据包进行反转义 
					if ((ret = anti_escape(content, lenth, tmpContent, &tmpContentLenth)) < 0)
					{
						//myprint("Err : func anti_escape(), srcLenth : %d, dstLenth : %d", lenth, tmpContentLenth);
						goto End;
					}

					reqPackHead_t	&req = *(reqPackHead_t *)tmpContent;

					//3.将转义后的数据包进行处理
					if ((ret = read_data_proce(tmpContent, tmpContentLenth, index)) < 0)
					{
						//myprint("Err : func read_data_proce(), dataLenth : %d", tmpContentLenth);
						goto End;
					}
					else
					{
						//4. 数据包处理成功, 清空临时变量
						memset(content, 0, sizeof(content));
						memset(tmpContent, 0, sizeof(tmpContent));
						lenth = 0;
						flag = 0;
						tmp = content + lenth;
					}

				}
				else
				{
					//5.取出数据包内容, 去除起始标识符
					*tmp++ = recvBuf[i];
					lenth += 1;
					continue;
				}
			}


		End:

			return ret;
		}

		size_t read_complete(const boost::asio::const_buffer &buffer, const boost::system::error_code & ec, size_t bytes) {
			if (ec) 
				return 0;

			//至少要收满标志位+包头
			if (bytes < sizeof(reqPackHead_t) + 1)
				return sizeof(reqPackHead_t) + 1 - bytes;

			const char *pBuf = static_cast<const char *>(boost::asio::detail::buffer_cast_helper(buffer));

			int  tmpContentLenth = 0;			//转义后的数据长度
			char tmpContent[1400] = { 0 }; 		//转义后的数据

			//2. 跳过第一个标志位， 将数据包进行反转义 
			int ret = 0;
			if ((ret = anti_escape(pBuf + 1, bytes - 1, tmpContent, &tmpContentLenth)) < 0)
			{
				//myprint("Err : func anti_escape(), srcLenth : %d, dstLenth : %d", lenth, tmpContentLenth);
				return 0;
			}

			//继续收满包内容、校验和（int）、结束标志
			const reqPackHead_t &req = *(reqPackHead_t *)(tmpContent);
			int contLen = req.contentLenth;
			if (contLen == 0)
				return 0;

			int all = 1 + contLen + sizeof(int) + 1;
			if (bytes < all)
				return all -bytes;
			else
				return 0;
		}

		void VirtualScannerSession::do_read()
		{
			auto self(shared_from_this());
#if IF_USE_ASYNC
			socket_.async_read_some(boost::asio::buffer(data_),
#else
			boost::asio::async_read(socket_, boost::asio::buffer(data_),
				std::bind(read_complete, boost::asio::buffer(data_), std::placeholders::_1, std::placeholders::_2),
#endif
				make_custom_alloc_handler(allocator_,
					[this, self](boost::system::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					//assert(typeid(socket_.get_io_service()) == typeid(boost::asio::stream_socket_service<tcp>));
#if IF_USE_ASYNC			
					auto service = (boost::asio::stream_socket_service<tcp> *)&socket_.get_io_service();
					
#if defined(BOOST_ASIO_HAS_IOCP)
					boost::asio::detail::win_iocp_socket_service<tcp>::implementation_type t;
#else
					boost::asio::detail::reactive_socket_service<tcp>::implementation_type t;
#endif
					service->native_handle(t);

					do
					{
						std::lock_guard<std::mutex> lock(_mtxSock2Data);
						if (_mapSock2Data.size() < 100)
							break;

						//消息满了，等待 or just close socket
						std::cout << "_mapSock2Data满了\n";

					} while (std::this_thread::sleep_for(std::chrono::seconds(1)), true);

					//先缓存，考虑封装成一个mgr类
					{
						std::lock_guard<std::mutex> lock(_mtxSock2Data);
						auto itr = _mapSock2Data.find(t.socket_);
						if (itr == std::end(_mapSock2Data))
						{
							deq_data_t	deq;

							deq.push_back(std::make_pair(MSG_TYPE::Read_MSG, data_));
							_mapSock2Data[t.socket_] = std::move(deq);
						}
						else
						{
							auto &deq = _mapSock2Data[t.socket_];
						/*	std::for_each(std::begin(deq), std::end(deq), [](auto data) {

							});*/

							deq.push_back(std::make_pair(MSG_TYPE::Read_MSG, data_));
						}
					}

					bool bNeedRead = false;
					//parse data
					
					bNeedRead = true;
					if(bNeedRead)
					{
						//没收完整，继续收
						do_read();
					}
					else
						do_write(length);
#else
					//解析包内容，并写入缓存。
					if (0 > find_whole_package(data_.data(), length, 0))
						return;//do_read();

					////获得需要写的长度
					//int nSockFD = getSocketFD();
					//int nlen2write = 0;

					//{
					//	//只有自己的socket，因为buffer是跟session、socket绑定的。
					//	std::lock_guard<std::mutex> lock(_mtxSock2Data);
					//	auto itr = _mapSock2Data.find(nSockFD);
					//	if (itr == std::end(_mapSock2Data))
					//		return;	//结束，没有可写的了

					//	auto &data = itr->second.second;
					//	nlen2write = itr->second.first;
					//	data_ = data;
					//	_mapSock2Data.erase(itr);
					//}

					do_write(nLen2Write_);
#endif
				}
			}));
		}

		void VirtualScannerSession::do_write(std::size_t length)
		{
			//boost::asio::async_write
			auto self(shared_from_this());
			boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
				make_custom_alloc_handler(allocator_,
					[this, self](boost::system::error_code ec, std::size_t /*length*/)
			{
				if (!ec)
				{
					//assert(typeid(socket_.get_io_service()) == typeid(boost::asio::stream_socket_service<tcp>));
					
					

					do_read();
				}
			}));
		}

		int VirtualScannerSvr::init()
		{
			//1.查看上传目录是否存在



			return 0;
		}

		GA2NDIDSVRLIB_API int main1(int argc, char* argv[])
		{
			try
			{
				if (argc != 2)
				{
					std::cerr << "Usage: server <port>\n";
					return 1;
				}
				
				boost::asio::io_service	io_service;

				/*wchar_t * pEnd;
				long int port;
				port = std::wcstol(argv[1], &pEnd, 10);*/
				int port = std::atoi(argv[1]);
				VirtualScannerSvr		server(io_service, port);

				std::vector<std::thread>	vctThreads;
				int	nThreadCount = 1;

				auto handle_thread = [&]()
				{
					io_service.run();
				};

				for (int i = 0; i < nThreadCount; ++i)
					vctThreads.push_back(std::thread(handle_thread));

				std::for_each(std::begin(vctThreads), std::end(vctThreads), [](auto &t) {t.join(); });

			}
			catch (std::exception& e)
			{
				std::cerr << "Exception: " << e.what() << "\n";
			}

			return 0;
		}
	}

	GA2NDIDSVRLIB_API int main2(int argc, wchar_t* argv[])
	{

		return 0;
	}
}





