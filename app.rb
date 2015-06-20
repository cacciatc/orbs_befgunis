require 'sinatra'

get '/' do
	send_file './public/pages/index.html'
end
