[% USE date %]
<html lang="en-GB">
    <head>
        <meta charset="UTF-8">
    </head>
    <body>
        <table>
        <tr><th>No</th><th>Id</th><th>Address</th><th>Lat</th><th>Lon</th><th>Time</th><tr>
        [% FOREACH user IN users %]
            <tr>
                <td>[% loop.count %]</td>
                <td><a href="/position?id_hash=[% user.id_hash %]">[% user.id_hash %]</a></td>
                <td>[% user.address %]</td>
                <td>[% user.lat %]</td>
                <td>[% user.lon %]</td>
                <td>[% date.format( user.ts ) %]</td>
            <tr>
        [% END %]
        </table>
    </body>
</html>
