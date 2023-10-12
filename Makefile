SRC_DIR = chat
C_SRC = \
	$(SRC_DIR)/private_message.cpp \
	$(SRC_DIR)/broadcast_message.cpp \
	$(SRC_DIR)/chat_user.cpp \
	$(SRC_DIR)/chat_client.cpp \
	$(SRC_DIR)/project_lib.cpp \
	$(SRC_DIR)/config_file.cpp \
	$(SRC_DIR)/client.cpp
S_SRC = \
	$(SRC_DIR)/private_message.cpp \
	$(SRC_DIR)/broadcast_message.cpp \
	$(SRC_DIR)/chat_user.cpp \
	$(SRC_DIR)/config_file.cpp \
	$(SRC_DIR)/chat_server.cpp \
	$(SRC_DIR)/SHA256.cpp \
	$(SRC_DIR)/project_lib.cpp \
	$(SRC_DIR)/server.cpp

C_TARGET = bin/chat
S_TARGET = bin/chat_server
PREFIX = /usr/local/bin
STD = c++20

chat: $(C_SRC) $(S_SRC) build_client build_server

build_client:
	g++ --std=$(STD) -o $(C_TARGET) $(C_SRC) -L.

build_server:
	g++ --std=$(STD) -o $(S_TARGET) $(S_SRC) -L.

clean:
	rm -rf *.o $(C_TARGET) $(S_TARGET)

install:
	install $(C_TARGET) $(PREFIX)
	install $(S_TARGET) $(PREFIX)

uninstall:
	rm -rf $(PREFIX)/$(C_TARGET)
	rm -rf $(PREFIX)/$(S_TARGET)
