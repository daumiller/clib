# clib #

C helper library

## Modules ##
<table><tbody>
  <tr><th> arguments  </th><td> command line argument parsing </td></tr>
  <tr><th> asock      </th><td> asynchronous sockets          </td></tr>
  <tr><th> base64     </th><td> base64 encoding/decoding      </td></tr>
  <tr><th> fileSystem </th><td> file/directory/path helpers   </td></tr>
  <tr><th> hash       </th><td> hash table                    </td></tr>
  <tr><th> list       </th><td> generic list                  </td></tr>
  <tr><th> regex      </th><td> regular expressions           </td></tr>
  <tr><th> sha256     </th><td> SHA2/SHA256 hashing           </td></tr>
  <tr><th> string     </th><td> strings and stringBuilders    </td></tr>
  <tr><th> threadPool </th><td> worker thread pooling         </td></tr>
</tbody></table>

## Platforms ##
  + OS X
  + Linux

## Dependencies ##
  + C99 mode
  + [PCRE](http://www.pcre.org/) (required; for regular expressions)
  + [OpenSSL](http://www.openssl.org/) (optional; for asock SSH tunneling)
  + [libssh2](http://www.libssh2.org/) (optional; for asock SSH tunneling)

## License ##
  + BSD
