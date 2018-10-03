#include <iostream>
#include <string>
#include <unordered_map>//best container
#include <map> //for median vals
#include <fstream>//file input
#include <sstream>//parsing lines
#include <queue>//priority queue
#include <vector>//for pqueue
#include <algorithm>//for min&max

using namespace std;

//summary of exchange info
struct Summary {
	int commission_earnings;
	int money_transferred;
	int completed_trades;
	int shares_traded;

	Summary() {
		commission_earnings = 0;
		money_transferred = 0;
		completed_trades = 0;
		shares_traded = 0;
	}

	void print() {
		cout << "Commission Earnings: $" + to_string(commission_earnings) << endl;
		cout << "Total Amount of Money Transferred: $" + to_string(money_transferred) << endl;
		cout << "Number of Completed Trades: " + to_string(completed_trades) << endl;
		cout << "Number of Shares Traded: " + to_string(shares_traded) << endl;
	}
};

//contain info for each order
struct Order {
	int ID;
	string client_name;
	string ticker;
	int price;
	int quantity;

	Order() {
		ID = 0;
		client_name = "";
		ticker = "";
		price = 0;
		quantity = 0;
	}

	Order(int ID_in, string client_in, string ticker_in, int price_in, int quant_in) {
		ID = ID_in;
		client_name = client_in;
		ticker = ticker_in;
		price = price_in;
		quantity = quant_in;
	}

	void print() {
		cout << "Order ID: " << ID << endl;
		cout << "Client: " << client_name << endl;
		cout << "Ticker: " << ticker << endl;
		cout << "Price: " << price << endl;
		cout << "Qty: " << quantity << endl;
	}
};

//comparitor for sells priority queue. place lowest sale price at top, settle ties with lower ID num
struct sell_sort {
	bool operator() (const Order& order1, const Order& order2) const {
		if (order1.price < order2.price) return false;
		if (order1.price > order2.price) return true;
		
		//else
		return order1.ID > order2.ID;
	}
};

//comparitor for buys priority queue. place highest buy price at top, settle ties with lower ID num
struct buy_sort {
    bool operator() (const Order& o1, const Order& o2) const {
        if (o1.price < o2.price) return true;
        if (o1.price > o2.price) return false;

        //else
        return o1.ID > o2.ID;
    }
};
		
//struct for all client info
struct Client {
    string name;
	int stocks_bought;
	int stocks_sold;
	int net_value_traded;

	Client() {
        name = "";
		stocks_bought = 0;
		stocks_sold = 0;
		net_value_traded = 0;
	}
	
	Client(string name_in, bool insider_in = false) {
        name = name_in;
		stocks_bought = 0;
		stocks_sold = 0;
		net_value_traded = 0;
	}

	void print() {
		cout << name << " bought " << stocks_bought << " and sold " << stocks_sold << " for a net transfer of $" << net_value_traded << endl;
	}
};

//pull median value from vector of sale values
int getMedianPrice(vector<int> values) {
	//sort
	sort(values.begin(), values.end());

	if(values.size() % 2 == 0 && values.size() != 0) {
		//even number of values, average middle 2 and truncate per requirements
		int medianVal = (values[(values.size() / 2) - 1] + values[values.size() / 2]) / 2;
		return medianVal;
	}
	else if (values.size() != 0) {
		//odd number of vals, return middle no.
		return values[values.size() / 2];
	}

	return 0;
}

//execute any potential trades. all passed arguments may be modified during function
void executeTrades(
    unordered_map<string,priority_queue<Order, vector<Order>, buy_sort>> &buys,
    unordered_map<string,priority_queue<Order, vector<Order>, sell_sort>> &sells,
    unordered_map<string,vector<int>> &medianValues,
    unordered_map<string,Client> &clients,
    Summary &market_summary
    ) {
    //continue to check for trades until none exist
    bool tradeExecuted = false;
    do {
        tradeExecuted = false;
        //loop through each ticker
        for(auto it = buys.begin(); it != buys.end(); ++it) {
            if(it->second.size() > 0 && sells[it->first].size() > 0 && sells[it->first].top().price <= it->second.top().price) {
                //execute trade at sale price
                tradeExecuted = true;

                Order buyOrder = it->second.top();
                Order sellOrder = sells[it->first].top();

                int sale_ID = min(buyOrder.ID,sellOrder.ID);
                int sale_price = 0;
                if (buyOrder.ID < sellOrder.ID) sale_price = buyOrder.price;
                else sale_price = sellOrder.price;
                int sale_qty = min(buyOrder.quantity,sellOrder.quantity);
                cout << buyOrder.client_name << " purchased " << sale_qty << " shares of " <<
                    it->first << " from " << sellOrder.client_name << " for $" << sale_price <<
                    "/share" << endl;

                //modify buy order
                if(buyOrder.quantity - sale_qty <= 0) it->second.pop();
                else {
                    buyOrder.quantity -= sale_qty;
                    it->second.pop();
                    it->second.push(buyOrder);
                }

                //modify sell order
                if(sellOrder.quantity - sale_qty <= 0) sells[it->first].pop();
                else {
                    sellOrder.quantity -= sale_qty;
                    sells[it->first].pop();
                    sells[it->first].push(sellOrder);
                }
                
                //take commission, 1% from buyer & seller
                int commission = (sale_price * sale_qty) / 100;
                
                //update summary
                market_summary.commission_earnings += (2 * commission);
                market_summary.money_transferred += (sale_price * sale_qty);
                market_summary.completed_trades++;
                market_summary.shares_traded += sale_qty;

                //update buy_client info
                clients[buyOrder.client_name].stocks_bought += sale_qty;
                clients[buyOrder.client_name].net_value_traded -= (sale_qty * sale_price);

                //update sell_client info
                clients[sellOrder.client_name].stocks_sold += sale_qty;
                clients[sellOrder.client_name].net_value_traded += (sale_qty * sale_price);

                //update median
                medianValues[it->first].push_back(sale_price);
            }
        }
    } while(tradeExecuted);
}

//main
int main(int argc, char* argv[]){

	bool summaryFlag = false;
	bool verboseFlag = false;
	bool medianFlag = false;
	bool transfersFlag = false;
	bool helpFlag = false;
	
	//input file
	ifstream input_file;

	//parse command line options
	for(int i = 1; i < argc; i++) {
		//get file path
		if (i == 1) {
			input_file.open(string(argv[i]));
		}

		//check flags
		if(string(argv[i]) == "-s" || string(argv[i]) == "--summary") summaryFlag = true;
		else if(string(argv[i]) == "-v" || string(argv[i]) == "--verbose") verboseFlag = true;
		else if(string(argv[i]) == "-m" || string(argv[i]) == "--median") medianFlag = true;
		else if(string(argv[i]) == "-t" || string(argv[i]) == "--transfers") transfersFlag = true;
		else if(string(argv[i]) == "-h" || string(argv[i]) == "--help") helpFlag = true;
	}

	//send help message and exit if necessary
	if (helpFlag) {
		//print a friendly help message
		cout << "Usage:" << endl;
        cout << "Exchange.app [file] [arguments]" << endl;
        cout << endl << "Arguments:" << endl << endl;
        cout << "   -s OR --summary: Print summary of market info at end of day." << endl;
        cout << "   -v OR --verbose: Print each trade as it is processed." << endl;
        cout << "   -m OR --median: Print median values of each ticker when when timestamp increases" << endl;
        cout << "   -t OR --transfers: Print all transfers for each client at end of day." << endl;
        cout << "   -h OR --help: Print this helpful information..." << endl;
		cout << endl;
        exit(0);
	}

	//Start market
	Summary market_summary = Summary();

	//map of ticker name to buy & sell orders
	unordered_map<string,priority_queue<Order, vector<Order>, buy_sort>> buys;
	unordered_map<string,priority_queue<Order, vector<Order>, sell_sort>> sells;
	
    //map of client names to clients; assumption: all client names are unique
    unordered_map<string,Client> clients;

    //map of tickers to sale values. For calculating median values
	unordered_map<string,vector<int>> medianValues;

	//parse input
	if(input_file.is_open()) {
		string line = "";
		string sub_line = "";
		int field_count = 0;
		int currentID = 0;
        int currentTime = 0;
        int newTime = 0;
		while (getline(input_file, line)) {
			//check input type
			if (line == "TL") continue;//header
			istringstream iss(line);
			Order newOrder = Order();
			bool Buy = false;
			field_count = 0;
			while(iss >> sub_line) {
                newOrder.ID = currentID;
                //time of order
				if (field_count == 0) newTime = stoi(sub_line);
				
                //client name
                if (field_count == 1) {
					newOrder.client_name = sub_line;
					Client newClient = Client(sub_line);
					clients[newOrder.client_name] = newClient;
				}

                //buy or sell order
				if (field_count == 2) {
					if (sub_line != "BUY" && sub_line != "SELL") {
						cout << "Invalid order provided:" << endl << line << endl;
						exit(1);
					}
					else if (sub_line == "BUY") Buy = true;
					else if (sub_line == "SELL") Buy = false;
				}

                //ticker for order
				if (field_count == 3) newOrder.ticker = sub_line;
				
                //price or buy or sell
                if (field_count == 4) {
					if (sub_line[0] != '$') {
						cout << "Invalid order provided:" << endl << line << endl;
						exit(1);
					} else {
						newOrder.price = stoi(sub_line.substr(1,sub_line.length() - 1));
					}
				}

                //quantity of buy or sell
				if (field_count == 5) {
					if (sub_line[0] != '#')	{
						cout << "Invalid order provided:" << endl << line << endl;
						exit(1);
					} else {
						newOrder.quantity = stoi(sub_line.substr(1,sub_line.length() - 1));
					}
				}
				field_count++;	
			}

			//add to buy or sells
			if (Buy) {
				buys[newOrder.ticker].push(newOrder);
			} else {
				sells[newOrder.ticker].push(newOrder);
			}

			//attempt to perform trades...
			if (currentTime != newTime) {
				//print median if necessary
				if(medianFlag) {
					for(auto it = medianValues.begin(); it != medianValues.end(); ++it) {
						cout << "Median match price of " << it->first << " at time " << currentTime << " is $" << getMedianPrice(it->second) << endl;
					}
				}
				 
                executeTrades(buys,sells,medianValues,clients,market_summary);
                
				//update timestamp
				currentTime = newTime;

			}

            //Update ID (one ID per order)
            currentID++;
				
		}
		input_file.close();
	}
	else {
		cout << "Unable to open input file..." << endl;
		exit(1);
	}

    //attempt to execute trades before end of day
    executeTrades(buys,sells,medianValues,clients,market_summary);

	//END OF DAY
	cout << "---End of Day---" << endl;
	
	if (summaryFlag) {
		market_summary.print();
	}

	if (transfersFlag) {
		for(auto it = clients.begin(); it != clients.end(); ++it) {
			it->second.print();
		}
	}
}
