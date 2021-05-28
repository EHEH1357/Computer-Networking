import socket
from socket import AF_INET, SOCK_DGRAM
import time

#socket creation
def socket_creation():
        clientSocket = socket.socket(AF_INET,SOCK_DGRAM)
        clientSocket.settimeout(1)
        return clientSocket

#client request
def client_request(clientSocket, select, serverName, serverPort):
        time.sleep(0.1)
        if select == "ping":
                message = "ping"
        else:
                message = input("message : ")
	#변수에 현재시간 할당
        start_time = time.time()
	#서버로 메세지 보내기
        clientSocket.sendto(message.encode(),(serverName, serverPort))
        return clientSocket, start_time

#receive request from server
def receive_request_from_server(clientSocket, start_time, num):
        try:
                message, address = clientSocket.recvfrom(1024)
                elapsed = calculate_rtt(start_time)
                print (str(message.decode()) + "    " + str(sequence_number) +  "    RTT: " + str(elapsed) + " sec")

        #소켓이 1초 이상 걸릴 경우, try 소켓 대신 실행
        except socket.timeout:
                print (sequence_number)
                print ("Request timed out")

#calculate rtt
def calculate_rtt(start_time):
        # 시작 시간 이후 경과 시간 계산
        elapsed = (time.time()-start_time)
        elapsed = float("{0:.5f}".format(elapsed))
        return elapsed

#close socket
def close_socket(clientSocket):
        print ("Socket Closed!!")
        clientSocket.close()

print ("Running")
print ("===============================")

serverName = 'localhost'
serverPort = 12000
socketcreation = socket_creation()
#PING 또는 message를 입력할 지 선택
choice = input("choose ping or message : ")

sequence_number = 1#반복 회수를 표시할 변수
while sequence_number<=10:
        CS, st = client_request(socketcreation, choice, serverName, serverPort)
        receive_request_from_server(CS, st, sequence_number)
        print ("")
        sequence_number+=1 

        if sequence_number > 10: #10번 실행 이후 소켓 닫기

                close_socket(socketcreation)
