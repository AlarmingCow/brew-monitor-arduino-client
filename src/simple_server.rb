require 'socket' 

server = TCPServer.new 8080

loop do
  socket = server.accept
  
  puts "Incoming request (#{Time.new}): \n" +
       "============================================"
  while line = socket.gets
    puts line
    break if line.strip.length == 0
  end
  puts "============================================\n\n"
  
  response = "Request received\n"

  socket.print "HTTP/1.1 200 OK\r\n" +
               "Content-Type: text/plain\r\n" +
               "Content-Length: #{response.bytesize}\r\n" +
               "Connection: close\r\n" +
	       "\r\n" +
	       response 
  socket.close
end
