## Order Book Data Structure Design

### Orders DS:
- `std::unordered_map<long long, std::array>`
- the long long type here is the order reference number (which is unique for any order)
- The value of the unordered_map is a std::array of fixed size 3, which contains [side, price, volume]

### Price Levels DS:
- `std::unordered_map<std::tuple, std::array>`
- The `std::tuple` key of the unordered map is a fixed size tuple of 2 values being (side, price)
- The std::array value of the unordered_map is a fixed size 2 array of [volume, order_count]


#### Add order:
- First, we add the order in the Order unorded_map
- Next, we update the price level unordered_mapvolume and order_count

#### Cancel order:
- First we delete from the orders unordered_map
- Next, we decrement the volume and order_count from the pirce levels unordered_map (and delete the entry if the volume is 0)

#### Execute order:
- If the order execution is the entire order volume, we call cancel order, if not do the below steps:
- Update the orders unordered_map to change the volume
- Update the price levels unordered_map to decrement the volume