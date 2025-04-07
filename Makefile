
# Определение компилятора
CXX := g++

# Файлы с исходным кодом
SRCS := main.cpp

# Объектные файлы
OBJS := main.o

# Выходной исполняемый файл
TARGET := program

# Флаги компилятора
CXXFLAGS := -Wall -Wextra -std=c++17

# Правило по умолчанию
all: $(TARGET)

# Компиляция программы
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Правило для компиляции .o файлов из .cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Очистка
clean:
	rm -f $(TARGET) $(OBJS)
