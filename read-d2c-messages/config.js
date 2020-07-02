module.exports = {
        knex: {
                client: 'pg',
                connection: {
                        user: "music_festival",
            password: "p@ssw0rd",
            host: "localhost",
            port: "5432",
            database: 'music_festival'
                },
                useNullAsDefault: true
        },
        defaultItems: [
                {name: 'beer', quantity: 60},
                {name: 'koozies', quantity: 103},
                {name: 'openers', quantity: 9}
        ]
}
