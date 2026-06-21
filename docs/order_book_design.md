## Order Book Data Structure Design

### Orders:
- unordered_map<long long, std::array>
- the long long type here is the order reference number (which is unique for any order)
- The value of the unordered_map is a std::array of fixed size 3, which contains [side, price, volume]

