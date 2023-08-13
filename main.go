package main

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/google/uuid"
	"github.com/zmb3/spotify/v2"
	spotifyauth "github.com/zmb3/spotify/v2/auth"
	"golang.org/x/oauth2"
)

const (
	port             = 8888
	tokenFile        = "access_token"
	clientIDFile     = "client_id"
	clientSecretFile = "client_secret"
)

func main() {
	ctx := context.Background()

	clientID, err := os.ReadFile(clientIDFile)
	if err != nil {
		fmt.Printf("Can't read client ID file: %v\n", err)
		return
	}
	clientSecret, err := os.ReadFile(clientSecretFile)
	if err != nil {
		fmt.Printf("Can't read client secret file: %v\n", err)
		return
	}

	redirectURL := fmt.Sprintf("http://localhost:%v/callback", port)
	auth := spotifyauth.New(
		spotifyauth.WithClientID(string(clientID)),
		spotifyauth.WithClientSecret(string(clientSecret)),
		spotifyauth.WithRedirectURL(redirectURL),
		spotifyauth.WithScopes(spotifyauth.ScopeUserReadCurrentlyPlaying),
	)

	rt, err := os.ReadFile(tokenFile)
	if err != nil && !errors.Is(err, os.ErrNotExist) {
		fmt.Printf("Error reading refresh token file: %v\n", err)
	}
	var token *oauth2.Token
	if len(rt) == 0 {
		// There's no previously-saved token, launch a server to collect an access code.
		state := uuid.NewString()
		url := auth.AuthURL(state)
		fmt.Printf("GO HERE TO AUTHENTICATE: \n%v\n\n", url)

		tc := make(chan string)
		http.HandleFunc("/callback", func(w http.ResponseWriter, r *http.Request) {
			io.WriteString(w, "Successfully authenticated! You can close this tab now.")
			tc <- r.URL.Query().Get("code")
		})

		srv := http.Server{
			Addr: fmt.Sprintf(":%d", port),
		}
		go srv.ListenAndServe()

		// Ensure we get the code before closing the server.
		code := <-tc
		srv.Close()

		if code == "" {
			fmt.Println("NO CODE AHH")
			return
		}
		token, err = auth.Exchange(ctx, code)
		if err != nil {
			fmt.Printf("Error Exchanging code: %v", err)
			return
		}
	} else {
		token = new(oauth2.Token)
		if err := json.Unmarshal(rt, token); err != nil {
			fmt.Println(err)
			return
		}
	}

	client := spotify.New(auth.Client(ctx, token))
	if err := wrtieClientTokenToFile(client); err != nil {
		fmt.Println(err)
		return
	}
	printInfo(ctx, client)
}

func printInfo(ctx context.Context, client *spotify.Client) {
	ticker := time.NewTicker(time.Second)
	title := ""
	for {
		playing, err := client.PlayerCurrentlyPlaying(ctx)
		if err != nil {
			fmt.Printf("Error getting now playing info: %v", err)
			return
		}
		if newTitle := playing.Item.Name; newTitle != title {
			artists := artistNames(playing.Item.Artists)
			fmt.Printf("%v\n%v\n\n", playing.Item.Name, artists)
			title = newTitle
		}

		<-ticker.C
	}
}

func artistNames(as []spotify.SimpleArtist) string {
	var names []string
	for _, a := range as {
		names = append(names, a.Name)
	}
	return strings.Join(names, ", ")
}

func wrtieClientTokenToFile(client *spotify.Client) error {
	token, err := client.Token()
	if err != nil {
		return err
	}
	j, err := json.MarshalIndent(token, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(tokenFile, j, 0666)
}
