SRC_DIR = chat
SRC = $(SRC_DIR)/private_message.cpp $(SRC_DIR)/broadcast_message.cpp $(SRC_DIR)/chat_user.cpp $(SRC_DIR)/chat_mgr.cpp $(SRC_DIR)/main.cpp
TARGET = bin/chat
PREFIX = /usr/local/bin

chat: $(SRC) build

build:
	g++ --std=c++20 -o $(TARGET) $(SRC) -L.

clean:
	rm -rf *.o $(TARGET)

install:
	install $(TARGET) $(PREFIX)

uninstall:
	rm -rf $(PREFIX)/$(TARGET)
