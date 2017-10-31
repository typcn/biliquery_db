# biliquery_db
Bilibili comment hash to user id rainbow table system


# Generate table
```php -f gen.php 1 300000000 > table```


# Run server
```
pacman -S libev 
make
./biliquery
```
About 15GB RAM needed

# API ( no sla, 300billon )

http://biliquery.typcn.com/api/user/hash/