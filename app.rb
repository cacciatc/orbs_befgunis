require 'sinatra'
require 'posix_mq'
require 'date'

configure do
$MQ = POSIX_MQ.new "/orbs-befungis-requests", :
          w
          end

          get '/' do
              send_file './public/pages/index.html'
              end

              get '/api/fungals' do
                  $MQ << "1234567890123456><"
                  end
