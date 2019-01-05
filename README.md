# p2p_file_transfer
Hi!
<br>
<br>
Compile:
<br>
&ensp;&ensp;+ Với index server: <br>
&ensp;&ensp;&ensp;&ensp;cd vào thư mục src/index_server<br>
&ensp;&ensp;&ensp;&ensp;Chạy lệnh make<br>
&ensp;&ensp;+ Với client:<br>
&ensp;&ensp;&ensp;&ensp;cd vào thư mục src/peer<br>
&ensp;&ensp;&ensp;&ensp;Chạy lệnh make
<br>
<br>
Chạy chương trình:<br>
&ensp;&ensp;+ Với index server: <br>
&ensp;&ensp;&ensp;&ensp;Trong thư mục src/index_server, chạy lệnh ./server --debug (thêm --debug để hiện full log)<br>
&ensp;&ensp;+ Với client:<br>
&ensp;&ensp;&ensp;&ensp;Trong thư mục src/peer, chạy lệnh ./peer<br>
&ensp;&ensp;&ensp;&ensp;Điền địa chỉ IP của server, port là 6969<br>
&ensp;&ensp;&ensp;&ensp;Nhập help để xem hướng dẫn sử dụng
<br>
<br>
<b>Chú ý:</b> Có thể chạy nhiều peer ở trên cùng localhost, nhưng nên copy file peer đã compile ra nhiều thư mục khác nhau để chạy.
