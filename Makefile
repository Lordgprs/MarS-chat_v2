SRC_DIR = chat
SRC = $(SRC_DIR)/private_message.cpp $(SRC_DIR)/broadcast_message.cpp $(SRC_DIR)/chat_user.cpp $(SRC_DIR)/chat_mgr.cpp $(SRC_DIR)/main.cpp $(SRC_DIR)/SHA256.cpp
TARGET = bin/chat
PREFIX = /usr/local/bin
STD = c++20

chat: $(SRC) build

build:
	g++ --std=$(STD) -o $(TARGET) $(SRC) -L.

clean:
	rm -rf *.o $(TARGET)

install:
	install $(TARGET) $(PREFIX)

uninstall:
	rm -rf $(PREFIX)/$(TARGET)
