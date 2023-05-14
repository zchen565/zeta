## This is the most simple and neat implementation of a TCP (HTTP) Server

- HTTP with GET only (to echo)
- install RAGEL
- naive one does not support config file
- we use example.cc to instantiate an echo server for HTTP


















# 一些问题与解答
- 为什么需要读写缓冲，为什么放在一起

- buffer使用strign 还是 vector<char>

使用 std::string 作为缓冲区的优点：

方便的字符串操作：std::string 提供了许多方便的字符串操作函数，如连接、查找、替换等。如果你需要对缓冲区进行频繁的字符串操作，使用 std::string 可以更方便和直观地处理。

自动扩容：std::string 在需要时会自动进行内存扩容，使得缓冲区的管理更加简单。你可以通过 push_back()、append() 等成员函数向 std::string 中添加字符，而无需手动管理缓冲区的大小。

字符串转换：std::string 提供了便捷的字符串转换功能，如将整数或浮点数转换为字符串，或将字符串转换为数值类型。

使用 std::vector<char> 作为缓冲区的优点：

适用于二进制数据：如果你的缓冲区包含二进制数据而不仅仅是文本字符串，std::vector<char> 更适合用于处理原始字节数据，因为它没有字符串处理的限制。

直接访问元素：std::vector<char> 允许你通过索引直接访问缓冲区中的元素，对于某些特定需求可能更为方便和高效。

可预测的内存布局：std::vector<char> 在内存中以连续的字节序列存储数据，这样可以更好地支持底层 I/O 操作。

- 取容器的首地址

return &*buffer_.begin();

return std::addressof(*buffer_.begin());

return std::data(buffer_);

- buffer的视图